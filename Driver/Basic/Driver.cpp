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
    auto session = NetworkSession::instance();
    if (!session) {
        error = "NetworkSession instance is unavailable.";
        return false;
    }

    // 监听网络请求返回与用户切换信号
    connect(session, &NetworkSession::responsed, this, &BasicDeviceModule::onRequested);
    connect(session, &NetworkSession::userIdChanged, this, &BasicDeviceModule::onUserChanged);

    return true;
}

bool BasicDeviceModule::unload(QString& error) {
    QMutexLocker locker(&mutex_);
    tasks_.clear();
    responseToId_.clear();
    return true;
}

bool BasicDeviceModule::writeRecordFile(QString identifier, QString filename, QByteArray rawData) {
    {
        QMutexLocker locker(&mutex_);
        if (tasks_.contains(identifier)) {
            return false;
        }
        tasks_[identifier] = {
            .identifier = identifier,
            .filename = filename,
            .data = rawData,
        };
    }

    QThreadPool::globalInstance()->start([this, identifier]() {
        QString targetFilename;
        QByteArray dataToSave;
        bool isCanceled = false;

        // 1. 线程启动时的初始状态检查
        {
            QMutexLocker subLocker(&mutex_);
            if (!tasks_.contains(identifier) || tasks_[identifier].cancelRequested) {
                tasks_.remove(identifier);
                isCanceled = true;
            }
            else {
                targetFilename = tasks_[identifier].filename;
                dataToSave = tasks_[identifier].data;
            }
        }

        if (isCanceled) {
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
                    {
                        QMutexLocker subLocker(&mutex_);
                        tasks_.remove(identifier); // 写入失败，锁内安全移除
                    }
                    break;
                }
                totalWritten += written;
            }
            file.close();
        }
        else {
            errorString = file.errorString();
            {
                QMutexLocker subLocker(&mutex_);
                tasks_.remove(identifier);
            }
        }

        // 3. 统一在子线程末尾进行线程安全的信号分发
        if (isCanceled) {
            QMetaObject::invokeMethod(this, [this, identifier]() {
                emit canceled(identifier);
                }, Qt::QueuedConnection);
        }
        else if (!success) {
            QMetaObject::invokeMethod(this, [this, identifier, errorString]() {
                emit errorOccured(identifier, errorString);
                }, Qt::QueuedConnection);
        } else {
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
    QString filename;
    QByteArray fileData;
    {
        QMutexLocker locker(&mutex_);
        if (!tasks_.contains(identifier) || tasks_[identifier].cancelRequested)
            return;

        filename = tasks_[identifier].filename;
        fileData = tasks_[identifier].data;
    }

    auto session = NetworkSession::instance();
    if (session->cloudEnabled() && session->logined()) {
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
            tasks_[identifier].response = resp;
            responseToId_[resp] = identifier;
        } else {
            qCritical().noquote() << "[" << modname_ << "ERROR] upload operation is failed.";
            tasks_.remove(identifier);
            emit this->errorOccured(identifier, "Failed to initiate network request.");
        }
    } else {
        emit this->uploaded(identifier);

        QMutexLocker locker(&mutex_);
        tasks_.remove(identifier);
    }
}

// void BasicDeviceModule::startUpload(const QString& identifier) {
//     QString filename;
// 
//     // 1. 锁内获取文件名
//     {
//         QMutexLocker locker(&mutex_);
//         if (!tasks_.contains(identifier) || tasks_[identifier].cancelRequested)
//             return;
// 
//         filename = tasks_[identifier].filename;
//     }
// 
//     auto session = NetworkSession::instance();
//     if (session && session->cloudEnabled() && session->logined()) {
// 
//         // 2. 从本地缓存目录打开 QFile
//         QString cacheDir = Configuration::instance().get(Field::CacheDirectory).toString();
//         QString fullPath = cacheDir.isEmpty() ? filename : QString("%1/%2").arg(cacheDir, filename);
// 
//         // 必须在堆上创建，且不能手动 delete，交给 HttpSession 的 unbox 机制去管理生命周期
//         QFile* fileDevice = new QFile(fullPath);
//         if (!fileDevice->open(QIODevice::ReadOnly)) {
//             qCritical() << "[" << modname_ << "ERROR] Failed to open file for uploading:" << fileDevice->errorString();
//             delete fileDevice;
// 
//             QMutexLocker locker(&mutex_);
//             tasks_.remove(identifier);
//             emit this->errorOccured(identifier, "Failed to open local record file.");
//             return;
//         }
// 
//         // 3. 构建分块的头部信息
//         QVariantMap filePartHeader;
//         filePartHeader[HttpField::HEADER_Type] = HttpField::HEADER_Type_Customized;
//         filePartHeader[HttpField::HEADER_Type_Data] = QStringLiteral("Content-Disposition");
//         filePartHeader[HttpField::HEADER_Value] = QString("form-data; name=\"file\"; filename=\"%1\"")
//             .arg(filename).toUtf8();
// 
//         QVariantList headersList;
//         headersList.append(filePartHeader);
// 
//         // 4. 构建 Part 映射 (装载 Header 和 QFile 指针)
//         QVariantMap partMap;
//         partMap[HttpField::MULTIDATA_Header] = headersList;
//         // 注意：必须显式转为 QIODevice* 存入 QVariant，否则 canConvert<QIODevice*> 会失败
//         partMap[HttpField::MULTIDATA_Data] = QVariant::fromValue(static_cast<QIODevice*>(fileDevice));
// 
//         QVariantList partsList;
//         partsList.append(partMap);
// 
//         // 5. 组装成符合 HttpSession 解析的 FormData 容器
//         QVariantMap multipartMap;
//         multipartMap[HttpField::DATA_Kind] = HttpField::DATA_Kind_FormData;
//         multipartMap[HttpField::DATA] = partsList;
// 
//         QVariantMap payload;
//         payload[HttpField::DATA] = multipartMap;
// 
//         QString localUrl = NetworkAddress::address(NetworkAddress::User, "records");
//         auto sid = SerialCode_Grower++;
//         if (SerialCode_Grower >= SerialCode_Grower_MAX)
//             SerialCode_Grower = 0;
// 
//         // 发送请求
//         NetworkRespone* resp = session->request(Network::Request::Post, localUrl,
//             RequestCode::UserCodeStart + 500 + sid, payload);
// 
//         QMutexLocker locker(&mutex_);
//         if (resp) {
//             tasks_[identifier].response = resp;
//             responseToId_[resp] = identifier;
//         }
//         else {
//             qCritical().noquote() << "[" << modname_ << "ERROR] upload operation is failed.";
//             tasks_.remove(identifier);
//             emit this->errorOccured(identifier, "Failed to initiate network request.");
//             // 注意：如果 request 返回失败且没有成功构造，fileDevice 需要手动 delete 规避泄漏
//             // 但如果 session 内部抛异常前已经把 fileDevice 挂在了 multipart 树上，则由 Qt 管理。
//         }
//     }
//     else {
//         emit this->uploaded(identifier);
// 
//         QMutexLocker locker(&mutex_);
//         tasks_.remove(identifier);
//     }
// }

bool BasicDeviceModule::readRecordFile(QString identifier) {
    QVariant pathVar = Configuration::instance().get(modname_ 
        + Field::member(NetworkSession::instance()->userId()) 
        + Field::member(identifier));

    if (!pathVar.isValid() || pathVar.toString().isEmpty()) {
        return false;
    }

    QString filename = pathVar.toString();
    QThreadPool::globalInstance()->start([this, identifier, filename]() {
        QString cacheDir = Configuration::instance().get(Field::CacheDirectory).toString();
        QString fullPath = cacheDir.isEmpty() ? filename : QString("%1/%2").arg(cacheDir, filename);
        QFile file(fullPath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            QMetaObject::invokeMethod(this, [this, identifier, data]() {
                emit fileRead(identifier, data);
            }, Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(this, [this, identifier, err = file.errorString()]() {
                emit errorOccured(identifier, err);
            }, Qt::QueuedConnection);
        }
    });

    return true;
}

void BasicDeviceModule::onRequested(NetworkRespone* respone) {
    if (!respone) return;

    QString identifier;
    bool isCanceled = false;
    {
        QMutexLocker locker(&mutex_);
        if (!responseToId_.contains(respone))
            return;
        identifier = responseToId_.take(respone);

        if (tasks_.contains(identifier) && tasks_[identifier].cancelRequested) {
            isCanceled = true;
        }
    }

    if (isCanceled) {
        QMetaObject::invokeMethod(this, [this, identifier]() {
            emit canceled(identifier);
            }, Qt::QueuedConnection);
    } else if (respone->code() == ResponseCode::Success) {
        QMetaObject::invokeMethod(this, [this, identifier]() {
            emit uploaded(identifier);
            }, Qt::QueuedConnection);
    } else {
        QMetaObject::invokeMethod(this, [this, identifier, error = respone->error()]() {
            emit errorOccured(identifier, error);
            }, Qt::QueuedConnection);
    }

    {
        QMutexLocker locker(&mutex_);
        tasks_.remove(identifier);
    }
}

void BasicDeviceModule::onUserChanged() {
}