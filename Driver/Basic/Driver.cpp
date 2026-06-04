#include <QDir>
#include <QFile>
#include <QThreadPool>
#include <QMetaObject>
#include "Core/Configuration.hpp"
#include "Backend/Network/HttpField.hpp"
#include "Backend/Network/Session.hpp"
#include "Driver.hpp"

static constexpr int SerialCode_Grower_MAX = 5000;
static int SerialCode_Grower = 0;

namespace DriverBasicField {
    static const auto ROOT = Field::ROOT;
}

BasicDeviceModule::BasicDeviceModule(QString moduleName, int major, int minor, int patch)
    : DeviceModule(moduleName, major, minor, patch) {
    auto session = NetworkSession::instance();
    connect(session, &NetworkSession::responsed, this, &BasicDeviceModule::onRequested);
    connect(session, &NetworkSession::userIdChanged, this, &BasicDeviceModule::onUserChanged);

    QVariantMap map;
    auto root = DriverBasicField::ROOT + Field::member(moduleName);
    if (!assign(map, Configuration::instance().get(root))) {
        Configuration::instance().set(root, QVariantMap{});
    }
}

bool BasicDeviceModule::load(QString& error) {
    this->loadHistoryRecords();
    return true;
}

bool BasicDeviceModule::unload(QString& error) {
    {
        QMutexLocker locker(&mutex_);
        int abandonedTasks = tasks_.size();
        qInfo() << "[DeviceModule Unload] Module unloaded safely." << abandonedTasks << "tasks cleared.";
        tasks_.clear();
        responseToId_.clear();
    }
    recordsToFullPath_.clear();
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

        {
            QMutexLocker subLocker(&mutex_);
            if (!tasks_.contains(identifier) || tasks_.value(identifier).cancelRequested) {
                tasks_.remove(identifier);
                isCanceled = true;
            } else {
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

        QString cacheDir = Configuration::instance().get(Field::CacheDirectory).toString();
        QString fullPath = cacheDir.isEmpty() ? targetFilename : QString("%1/%2").arg(cacheDir, targetFilename);
        {
            QFileInfo c(fullPath);
            QDir w = QDir::current();
            if (!w.exists(c.absolutePath()))
                w.mkdir(c.absolutePath());
            fullPath = c.absoluteFilePath();
        }
        
        QFile file(fullPath);
        bool success = false;
        QString errorString;

        if (file.open(QIODevice::WriteOnly)) {
            qint64 totalWritten = 0;
            qint64 totalSize = dataToSave.size();
            const qint64 chunkSize = 4 * 1024 * 1024;
            success = true;

            while (totalWritten < totalSize) {
                {
                    QMutexLocker subLocker(&mutex_);
                    if (!tasks_.contains(identifier) || tasks_[identifier].cancelRequested) {
                        tasks_.remove(identifier);
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
                        tasks_.remove(identifier);
                    }
                    break;
                }
                totalWritten += written;
            }
            file.close();
        } else {
            success = false;
            errorString = file.errorString();
            qCritical() << "[DeviceModule ERROR]" << identifier << "write file to:" << fullPath << "error:" << errorString;
            {
                QMutexLocker subLocker(&mutex_);
                tasks_.remove(identifier);
            }
        }

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
            auto userId = NetworkSession::instance()->userId();
            Configuration::instance().set(Field::ROOT 
                + Field::member(modname_) 
                + (!userId.isEmpty() ? Field::member(userId) : lstr())
                + Field::member(identifier), fullPath);
            QMetaObject::invokeMethod(this, [this, identifier, fullPath]() {
                emit this->fileSaved(identifier);

                this->recordsToFullPath_.insert(identifier, fullPath);
                emit this->recordsChanged();

                this->startUpload(identifier);
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

QStringList BasicDeviceModule::availbleRecords() {
    return this->recordsToFullPath_.keys();
}

void BasicDeviceModule::loadHistoryRecords() {
    this->recordsToFullPath_.clear();
    auto session = NetworkSession::instance();
    QString userId = session->userId();
    qInfo() << "[DeviceModule] Loading historical records for user:" << (userId.isEmpty() ? lstr("[EMPTY]") : userId);

    QString userPrefix = Field::ROOT + Field::member(modname_) + (userId.isEmpty() ? lstr() : Field::member(userId));
    QString historyListKey = userPrefix;

    QVariantMap historyList = Configuration::instance().get(historyListKey).toMap();
    QMutexLocker locker(&mutex_);
    for (auto[identifier, fullpath]: historyList.asKeyValueRange()) {
        if (recordsToFullPath_.contains(identifier)) 
            continue;
        QVariant pathVar = Configuration::instance().get(userPrefix + Field::member(identifier));
        if (pathVar.isValid() && !pathVar.toString().isEmpty()) {
            recordsToFullPath_.insert(identifier, fullpath.toString());
            qInfo() << "[DeviceModule] Historical record restored:" << identifier << "->" << pathVar.toString();
        }
    }
    emit this->recordsChanged();
}

bool BasicDeviceModule::readRecordFile(QString identifier) {
    qDebug() << "[DeviceModule] Receive request to read record file. ID:" << identifier;
    auto userId = NetworkSession::instance()->userId();
    QVariant pathVar = Configuration::instance().get(Field::ROOT 
        + Field::member(modname_)
        + (!userId.isEmpty() ? userId : lstr())
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
        } else {
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
    QString currentUid = NetworkSession::instance()->userId();
    qInfo() << "[DeviceModule] User session changed detector triggered. Target User ID:" << currentUid;

    QMutexLocker locker(&mutex_);
    int clearedTasksCount = tasks_.size();
    if (clearedTasksCount > 0) {
        qWarning() << "[DeviceModule WARNING]" << clearedTasksCount << "incomplete tasks abandoned due to login user switch.";
        tasks_.clear();
        responseToId_.clear();
    }
    this->loadHistoryRecords();
}