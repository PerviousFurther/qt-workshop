#include <QDir>
#include <QFile>
// #include <QtConcurrent/QtConcurrentRun>
#include <QThreadPool>
#include <QMetaObject>
#include <QDebug>
#include "Core/Configuration.hpp"
#include "Backend/Network/HttpField.hpp"
#include "Backend/Network/Session.hpp"
#include "Driver.hpp"

static constexpr int SerialCode_Grower_MAX = 5000;
static int SerialCode_Grower = 0;

bool BasicDeviceModule::load(QString& error) {
    qInfo() << "[DeviceModule] Loading module...";
    auto session = NetworkSession::instance();
    if (!session) {
        error = "NetworkSession instance is unavailable.";
        qCritical() << "[DeviceModule ERROR] Load failed:" << error;
        return false;
    }

    // 监听网络请求返回与用户切换信号
    connect(session, &NetworkSession::responsed, this, &BasicDeviceModule::onRequested);
    connect(session, &NetworkSession::userIdChanged, this, &BasicDeviceModule::onUserChanged);

    qInfo() << "[DeviceModule] Module loaded successfully.";
    return true;
}

bool BasicDeviceModule::unload(QString& error) {
    qInfo() << "[DeviceModule] Unloading module. Clearing all remaining tasks...";
    QMutexLocker locker(&mutex_);
    int abandonedTasks = tasks_.size();
    tasks_.clear();
    responseToId_.clear();

    qInfo() << "[DeviceModule] Module unloaded safely." << abandonedTasks << "tasks cleared.";
    return true;
}

bool BasicDeviceModule::writeRecordFile(QString identifier, QString filename, QByteArray rawData) {
    qDebug() << "[DeviceModule] Receive request to write record file. ID:" << identifier << "Filename:" << filename;
    {
        QMutexLocker locker(&mutex_);
        if (tasks_.contains(identifier)) {
            qWarning() << "[DeviceModule WARNING] Task already exists for identifier:" << identifier;
            return false;
        }
        tasks_[identifier] = {
            .identifier = identifier,
            .filename = filename,
            .data = rawData,
        };
    }

    QThreadPool::globalInstance()->start([this, identifier]() {
        qInfo() << "[DeviceModule WORK] Start saving file:" << identifier;
        QString targetFilename;
        QByteArray dataToSave;
        bool isCanceled = false;

        // 1. 线程启动时的初始状态检查
        {
            QMutexLocker subLocker(&mutex_);
            if (!tasks_.contains(identifier) || tasks_.value(identifier).cancelRequested) {
                tasks_.remove(identifier);
                isCanceled = true;
            }
            else {
                targetFilename = tasks_[identifier].filename;
                dataToSave = tasks_[identifier].data;
            }
        }

        if (isCanceled) {
            qInfo() << "[DeviceModule WORK CANCELLED]" << identifier << " is cancelled at startup.";
            QMetaObject::invokeMethod(this, [this, identifier]() {
                emit canceled(identifier);
                }, Qt::QueuedConnection);
            return;
        }

        // 获取配置中的缓存目录
        QString cacheDir = Configuration::instance().get(Field::CacheDirectory).toString();
        QString fullPath = cacheDir.isEmpty() ? targetFilename : QString("%1/%2").arg(cacheDir, targetFilename);

        QFile file(fullPath);
        bool success = false;
        QString errorString;

        if (file.open(QIODevice::WriteOnly)) {
            qint64 totalWritten = 0;
            qint64 totalSize = dataToSave.size();
            const qint64 chunkSize = 4 * 1024 * 1024;
            success = true;

            while (totalWritten < totalSize) {
                // 2. 分块写入期间的取消检查
                {
                    QMutexLocker subLocker(&mutex_);
                    if (!tasks_.contains(identifier) || tasks_[identifier].cancelRequested) {
                        tasks_.remove(identifier); // 必须在锁内移除
                        isCanceled = true;
                    }
                }

                if (isCanceled) {
                    break;
                }

                qint64 toWrite = qMin(chunkSize, totalSize - totalWritten);
                qint64 written = file.write(dataToSave.constData() + totalWritten, toWrite);
                if (written != toWrite) {
                    success = false;
                    errorString = file.errorString();
                    qCritical() << "[DeviceModule ERROR]" << identifier << "Chunk write failed:" << errorString;
                    {
                        QMutexLocker subLocker(&mutex_);
                        tasks_.remove(identifier); // 写入失败，锁内安全移除
                    }
                    break;
                }
                totalWritten += written;
            }
            file.close();
        } else {
            success = false;
            errorString = file.errorString();
            qCritical() << "[DeviceModule ERROR]" << identifier << "write file occur" << errorString;
            {
                QMutexLocker subLocker(&mutex_);
                tasks_.remove(identifier);
            }
        }

        // 3. 统一在子线程末尾进行线程安全的信号分发
        if (isCanceled) {
            qInfo() << "[DeviceModule WORK CANCELLED]" << identifier << " is cancelled during chunk writing.";
            QMetaObject::invokeMethod(this, [this, identifier]() {
                emit canceled(identifier);
            }, Qt::QueuedConnection);
        } else if (!success) {
            QMetaObject::invokeMethod(this, [this, identifier, errorString]() {
                emit errorOccured(identifier, errorString);
            }, Qt::QueuedConnection);
        } else {
            qInfo() << "[DeviceModule WORK DONE] File saved successfully. ID:" << identifier << "Path:" << fullPath;
            Configuration::instance().set(modname_
                + Field::member(NetworkSession::instance()->userId())
                + Field::member(identifier), targetFilename);
            QMetaObject::invokeMethod(this, [this, identifier]() {
                emit fileSaved(identifier);
                startUpload(identifier);
            }, Qt::QueuedConnection);
        }
    });

    return true;
}

void BasicDeviceModule::startUpload(const QString& identifier) {
    qInfo() << "[DeviceModule] Preparing upload task for ID:" << identifier;
    QString filename;
    QByteArray fileData;
    {
        QMutexLocker locker(&mutex_);
        if (!tasks_.contains(identifier) || tasks_[identifier].cancelRequested) {
            qWarning() << "[DeviceModule WARNING] Upload aborted: task not found or cancel requested for ID:" << identifier;
            return;
        }

        filename = tasks_[identifier].filename;
        fileData = tasks_[identifier].data;
    }

    auto session = NetworkSession::instance();
    if (session && session->cloudEnabled() && session->logined()) {
        qInfo() << "[DeviceModule] Cloud network is active. Submitting request for ID:" << identifier;
        QVariantMap filePartHeader;
        filePartHeader[HttpField::HEADER_Type] = HttpField::HEADER_Type_Customized;
        filePartHeader[HttpField::HEADER_Type_Data] = lstr("Content-Disposition");
        filePartHeader[HttpField::HEADER_Value] = lstr("form-data; name=\"file\"; filename=\"%1\"")
            .arg(filename).toUtf8();

        QVariantList headersList;
        headersList.append(filePartHeader);

        QVariantMap partMap;
        partMap[HttpField::MULTIDATA_Header] = headersList;
        partMap[HttpField::MULTIDATA_Data] = fileData;

        QVariantList partsList;
        partsList.append(partMap);

        QVariantMap multipartMap;
        multipartMap[HttpField::DATA_Kind] = HttpField::DATA_Kind_FormData;
        multipartMap[HttpField::DATA] = partsList;

        QVariantMap payload;
        payload[HttpField::DATA] = multipartMap;

        QString localUrl = NetworkAddress::address(NetworkAddress::User, "records");
        auto sid = SerialCode_Grower++;
        if (SerialCode_Grower >= SerialCode_Grower_MAX)
            SerialCode_Grower = 0;

        NetworkRespone* resp = session->request(Network::Request::Post, localUrl,
            RequestCode::UserCodeStart + 500 + sid, payload);

        QMutexLocker locker(&mutex_);
        if (resp) {
            qDebug() << "[DeviceModule] Network request initiated successfully. SID:" << sid << "ID:" << identifier;
            tasks_[identifier].response = resp;
            responseToId_[resp] = identifier;
        }
        else {
            qCritical().noquote() << "[" << modname_ << "ERROR] upload operation is failed for ID:" << identifier;
            tasks_.remove(identifier);
            emit this->errorOccured(identifier, "Failed to initiate network request.");
        }
    }
    else {
        qInfo() << "[DeviceModule] Cloud disabled or user not logined. Fallback to local-only for ID:" << identifier;
        emit this->uploaded(identifier);

        QMutexLocker locker(&mutex_);
        tasks_.remove(identifier);
    }
}

/* // 备用版本 startUpload (基于 QFile 句柄流传输优化) 补充同步日志与信号
void BasicDeviceModule::startUpload(const QString& identifier) {
    qInfo() << "[DeviceModule Stream] Preparing stream upload for ID:" << identifier;
    QString filename;

    // 1. 锁内获取文件名
    {
        QMutexLocker locker(&mutex_);
        if (!tasks_.contains(identifier) || tasks_[identifier].cancelRequested) {
            qWarning() << "[DeviceModule Stream WARNING] Upload aborted: task not found or cancel requested for ID:" << identifier;
            return;
        }

        filename = tasks_[identifier].filename;
    }

    auto session = NetworkSession::instance();
    if (session && session->cloudEnabled() && session->logined()) {

        // 2. 从本地缓存目录打开 QFile
        QString cacheDir = Configuration::instance().get(Field::CacheDirectory).toString();
        QString fullPath = cacheDir.isEmpty() ? filename : QString("%1/%2").arg(cacheDir, filename);

        // 必须在堆上创建，且不能手动 delete，交给 HttpSession 的 unbox 机制去管理生命周期
        QFile* fileDevice = new QFile(fullPath);
        if (!fileDevice->open(QIODevice::ReadOnly)) {
            qCritical() << "[" << modname_ << "ERROR] Failed to open file for uploading:" << fileDevice->errorString();
            delete fileDevice;

            QMutexLocker locker(&mutex_);
            tasks_.remove(identifier);
            emit this->errorOccured(identifier, "Failed to open local record file.");
            return;
        }

        qInfo() << "[DeviceModule Stream] Initiating cloud multipart post stream for ID:" << identifier;
        // 3. 构建分块的头部信息
        QVariantMap filePartHeader;
        filePartHeader[HttpField::HEADER_Type] = HttpField::HEADER_Type_Customized;
        filePartHeader[HttpField::HEADER_Type_Data] = QStringLiteral("Content-Disposition");
        filePartHeader[HttpField::HEADER_Value] = QString("form-data; name=\"file\"; filename=\"%1\"")
            .arg(filename).toUtf8();

        QVariantList headersList;
        headersList.append(filePartHeader);

        // 4. 构建 Part 映射 (装载 Header 和 QFile 指针)
        QVariantMap partMap;
        partMap[HttpField::MULTIDATA_Header] = headersList;
        // 注意：必须显式转为 QIODevice* 存入 QVariant，否则 canConvert<QIODevice*> 会失败
        partMap[HttpField::MULTIDATA_Data] = QVariant::fromValue(static_cast<QIODevice*>(fileDevice));

        QVariantList partsList;
        partsList.append(partMap);

        // 5. 组装成符合 HttpSession 解析的 FormData 容器
        QVariantMap multipartMap;
        multipartMap[HttpField::DATA_Kind] = HttpField::DATA_Kind_FormData;
        multipartMap[HttpField::DATA] = partsList;

        QVariantMap payload;
        payload[HttpField::DATA] = multipartMap;

        QString localUrl = NetworkAddress::address(NetworkAddress::User, "records");
        auto sid = SerialCode_Grower++;
        if (SerialCode_Grower >= SerialCode_Grower_MAX)
            SerialCode_Grower = 0;

        // 发送请求
        NetworkRespone* resp = session->request(Network::Request::Post, localUrl,
            RequestCode::UserCodeStart + 500 + sid, payload);

        QMutexLocker locker(&mutex_);
        if (resp) {
            qDebug() << "[DeviceModule Stream] Network request initiated successfully. SID:" << sid << "ID:" << identifier;
            tasks_[identifier].response = resp;
            responseToId_[resp] = identifier;
        }
        else {
            qCritical().noquote() << "[" << modname_ << "ERROR] upload operation is failed for ID:" << identifier;
            tasks_.remove(identifier);
            emit this->errorOccured(identifier, "Failed to initiate network request.");
            // 注意：如果 request 返回失败且没有成功构造，fileDevice 需要手动 delete 规避泄漏
            // 但如果 session 内部抛异常前已经把 fileDevice 挂在了 multipart 树上，则由 Qt 管理。
        }
    }
    else {
        qInfo() << "[DeviceModule Stream] Cloud disabled or user not logined. Fallback to local-only for ID:" << identifier;
        emit this->uploaded(identifier);

        QMutexLocker locker(&mutex_);
        tasks_.remove(identifier);
    }
}
*/

bool BasicDeviceModule::readRecordFile(QString identifier) {
    qDebug() << "[DeviceModule] Receive request to read record file. ID:" << identifier;
    QVariant pathVar = Configuration::instance().get(modname_
        + Field::member(NetworkSession::instance()->userId())
        + Field::member(identifier));

    if (!pathVar.isValid() || pathVar.toString().isEmpty()) {
        qWarning() << "[DeviceModule WARNING] No record path configuration found for ID:" << identifier;
        return false;
    }

    QString filename = pathVar.toString();
    QThreadPool::globalInstance()->start([this, identifier, filename]() {
        qInfo() << "[DeviceModule WORK] Start reading file for ID:" << identifier << "Filename:" << filename;
        QString cacheDir = Configuration::instance().get(Field::CacheDirectory).toString();
        QString fullPath = cacheDir.isEmpty() ? filename : QString("%1/%2").arg(cacheDir, filename);
        QFile file(fullPath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            qInfo() << "[DeviceModule WORK DONE] File read successfully. ID:" << identifier << "Size:" << data.size() << "bytes";
            QMetaObject::invokeMethod(this, [this, identifier, data]() {
                emit fileRead(identifier, data);
                }, Qt::QueuedConnection);
        }
        else {
            QString err = file.errorString();
            qCritical() << "[DeviceModule ERROR] Failed to open file for reading. ID:" << identifier << "Error:" << err;
            QMetaObject::invokeMethod(this, [this, identifier, err]() {
                emit errorOccured(identifier, err);
                }, Qt::QueuedConnection);
        }
        });

    return true;
}

void BasicDeviceModule::onRequested(NetworkRespone* respone) {
    if (!respone) {
        qWarning() << "[DeviceModule WARNING] Received empty network response object.";
        return;
    }

    QString identifier;
    bool isCanceled = false;
    {
        QMutexLocker locker(&mutex_);
        if (!responseToId_.contains(respone)) {
            // 注意：该网络响应槽可能被多个业务监听，非本模块触发的请求直接过滤，属于正常逻辑，无需报错
            return;
        }
        identifier = responseToId_.take(respone);

        if (tasks_.contains(identifier) && tasks_[identifier].cancelRequested) {
            isCanceled = true;
        }
    }

    qInfo() << "[DeviceModule] Network slot captured response for ID:" << identifier << "Response Code:" << respone->code();

    if (isCanceled) {
        qInfo() << "[DeviceModule] Task was already requested to cancel. Discarding response for ID:" << identifier;
        QMetaObject::invokeMethod(this, [this, identifier]() {
            emit canceled(identifier);
            }, Qt::QueuedConnection);
    }
    else if (respone->code() == ResponseCode::Success) {
        qInfo() << "[DeviceModule] Cloud upload successfully completed for ID:" << identifier;
        QMetaObject::invokeMethod(this, [this, identifier]() {
            emit uploaded(identifier);
            }, Qt::QueuedConnection);
    }
    else {
        QString errorMsg = respone->error();
        qCritical() << "[DeviceModule ERROR] Server replied failure for ID:" << identifier << "Error:" << errorMsg;
        QMetaObject::invokeMethod(this, [this, identifier, errorMsg]() {
            emit errorOccured(identifier, errorMsg);
            }, Qt::QueuedConnection);
    }

    {
        QMutexLocker locker(&mutex_);
        tasks_.remove(identifier);
    }
}

void BasicDeviceModule::onUserChanged() {
    QString currentUid = NetworkSession::instance() ? NetworkSession::instance()->userId() : "Unknown";
    qInfo() << "[DeviceModule] User session changed detector triggered. Target User ID:" << currentUid;

    // 安全防范：切换账号时，旧账号未决的缓存映射和上传句柄必须立刻清洗，防止串道和脏数据上报
    QMutexLocker locker(&mutex_);
    int clearedTasksCount = tasks_.size();
    if (clearedTasksCount > 0) {
        qWarning() << "[DeviceModule WARNING]" << clearedTasksCount << "incomplete tasks abandoned due to login user switch.";
        tasks_.clear();
        responseToId_.clear();
    }
}