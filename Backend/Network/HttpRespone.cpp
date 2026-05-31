#include <QNetworkReply>
#include <QJsonDocument>
#include <QMetaEnum>

#include "HttpField.hpp"
#include "HttpRespone.hpp"
#include "HttpSession.hpp"

HttpResponse::HttpResponse(QNetworkReply* parent, HttpSession* session, int serialCode, int deadtime, bool read)
    : NetworkRespone(parent)
    , serialCode_{ serialCode }
    , sess_{ session }
    , deadTime_{ deadtime }
    , type_{ read } {
    connect(parent, &QNetworkReply::errorOccurred, this, &HttpResponse::handleError);
}

QString errorToString(QNetworkReply::NetworkError code) {
    auto metaEnum = QMetaEnum::fromType<QNetworkReply::NetworkError>();
    return QString::fromUtf8(metaEnum.valueToKey(code));
}

void HttpResponse::handleError(QNetworkReply::NetworkError error) {
    {
        QMutexLocker _(&this->mtx_);
        this->code_ = static_cast<int>(error);
        this->error_ = errorToString(error);
        qWarning() << lstr("request code %1 have some error, code: %2 message: %3")
            .arg(this->serialCode_).arg(this->code_).arg(this->error_);
    }
    emit this->responsed(this);
    emit sess_->responsed(this);
}

HttpResponse::~HttpResponse() {

};

// call from consumer's thread.
QVariantMap HttpResponse::get(bool waitEnd) {
    bool overtime = false;
    bool endOfLoop = false;
    qint64 currentSize = 0;
    qint64 totalSize = 0;
    int error;

    do {
        if (parent()->isReadable())
            overtime = parent()->waitForReadyRead(deadTime_);
        else
            overtime = parent()->waitForBytesWritten(deadTime_);
        error = parent()->error();
    } while (waitEnd && !endOfRequest());

    if (!overtime) {
        QMutexLocker _(&this->mtx_);
        currentSize = currentSize_;
        totalSize = totalSize_;
    }

    QVariantMap result;
    if (overtime) {
        qWarning() << lstr("SerialCode %1 read/get request is overtime.").arg(this->serialCode_);
        result = {
            {NetworkField::CODE, QVariant(ResponseCode::ClientOvertime)},
            {NetworkField::MSG, QVariant(ResponseString::ClientOvertime)}
        };
    }
    else if (error != QNetworkReply::NetworkError::NoError) {
        result = {
            {NetworkField::CODE, QVariant(error)},
            {NetworkField::MSG, QVariant(parent()->errorString())}
        };
    }
    else {
        auto bytes = parent()->readAll();
        if (bytes.isEmpty()) {
            result = {
                {NetworkField::CODE, QVariant(ResponseCode::Unknown)},
                {NetworkField::MSG, QVariant(ResponseString::UnknownErr)}
            };
        }
        else {
            QJsonParseError err;
            auto doc = QJsonDocument::fromJson(bytes, &err);
            if (err.error != QJsonParseError::NoError && !doc.isObject()) {
                result = {
                    {NetworkField::CODE, QVariant(ResponseCode::Success)},
                    {NetworkField::MSG,  QVariant(ResponseString::Success)},
                    {NetworkField::DATA, QVariant(qMove(bytes))},
                    {HttpField::ProgressSize, QVariant(currentSize)},
                    {HttpField::TotalSize, QVariant(totalSize)}
                };
            }
            else {
                result = fromJson(doc);
            }
        }
    }
    return result;
}

//void HttpResponse::set(QVariantMap instruction) {
//    qFatal("The network upload operation is not finished.");
//    bool overtime;
//    {
//        QMutexLocker _(&this->mtx_);
//        overtime = this->cv_.wait(&this->mtx_,
//            deadTime_ < 0 ? QDeadlineTimer(QDeadlineTimer::Forever) : QDeadlineTimer(this->deadTime_));
//
//        
//        if (overtime) 
//            qWarning("SerialCode %1 write/set request is overtime.", this->serialCode_);
//        else if (QString value; copy(value, instruction, NetworkField::SET_Kind)) {
//            if (instruction.contains(NetworkField::DATA)) {
//                if (QIODevice* device; value == NetworkField::SET_Kind_IoDevice && copy(device, instruction, NetworkField::DATA)) {
//                    // parent()->();
//                } else if (QByteArray data; value == NetworkField::SET_Kind_Bytes && copy(data, instruction, NetworkField::DATA)) {
//
//                }
//            } else {
//                
//            }
//            
//        } else 
//            qWarning("SerialCode %1 write/set request have unknown kind.", this->serialCode_);
//    }
//}

bool HttpResponse::stop(QString& error) try {
    parent()->abort();
    error = ResponseString::Success;
    return true;
} catch (...) {
    error = lstr("未知错误。");
    return false;
}

void HttpResponse::handleFinished() {
    {
        QMutexLocker _(&this->mtx_);
        this->code(parent()->error());
    }
    emit sess_->responsed(this);
    emit this->responsed(this);
}
void HttpResponse::handleProgress(qint64 size, qint64 total) {
    {
        QMutexLocker _(&this->mtx_);
        this->currentSize_ = size;
        this->totalSize_ = total;
    }
    emit this->responsed(this);
    emit sess_->responsed(this);
}

bool HttpResponse::endOfRequest() const noexcept {
    return parent()->isFinished();
}

QNetworkReply* HttpResponse::parent() const noexcept {
    return static_cast<QNetworkReply*>(this->QObject::parent());
}