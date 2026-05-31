#include <QNetworkReply>
#include <QHttpHeaders>
#include <QHttpMultiPart>
#include <QReadWriteLock>
#include <QJsonDocument>
#include <QJsonObject>

#include "Core/Utils.hpp"
#include "Core/Configuration.hpp"
#include "HttpField.hpp"
#include "HttpSession.hpp"
#include "HttpRespone.hpp"

static const auto MODULE_NAME = "Network";

// QObject property require const char*.
static constexpr auto PROPERTY_RESPONE = "^rsp";

namespace HttpField {
    static const auto ROOT = Field::ROOT + Field::member(lstr("Http"));

    static const auto LastUser = lstr("lastUser");
    static const auto LAST_USER = Field::ROOT + Field::member(LastUser);

    static const auto BaseUrl = lstr("url");
    static const auto BASE_URL = ROOT + Field::member(BaseUrl);

    // For request.

    static const auto Authorization = lstr("authorization");

    static const auto UserId = lstr("userId");
    static const auto Username = lstr("userName");
    static const auto UserPassage = lstr("passage");

    static const auto LAST_USER_ID = LAST_USER + Field::member(UserId);
    static const auto LAST_USER_PASSAGE = LAST_USER + Field::member(UserPassage);
    static const auto LAST_USER_AUTHORIZATION = LAST_USER + Field::member(Authorization);

    static const auto Users = lstr("users");
    static const auto USERS = ROOT + Field::member(Users);
}

namespace HttpAddress {
    static const auto DEFAULT_URL = lstr("https://127.0.0.1:8080");
    static const auto USER = lstr("/user");
}

namespace RequestCode {
    static constexpr auto Login = 0x12;
    static constexpr auto Logout = 0x13;
    static constexpr auto SetUsername = 0x14;
    static constexpr auto CreateUser = 0x15;
    static constexpr auto DeleteUser = 0x16;
}

static const auto DEFAULT_CFG = QVariantMap{
    {HttpField::Users, QVariant(QVariantList())},
    // {HttpField::LastUser, QVariant()},
    {HttpField::BaseUrl, QVariant(HttpAddress::DEFAULT_URL)},
};

QVariantMap atleastHttpField() {
    auto value = Configuration::instance().get(HttpField::ROOT);
    if (QVariantMap map; assign(map, value))
        if (QVariantList list; copy<QString>(map, HttpField::BaseUrl) && copy(list, map, HttpField::Users)) {
            if (int lastUserId; copy(lastUserId, map, HttpField::LastUser)) {
                if (lastUserId >= list.size()) {
                    Configuration::instance().set(HttpField::LAST_USER, QVariant());
                    map[HttpField::LastUser] = QVariant();
                }
            }  
            return map;
        }
    qWarning("Network Session's configuration is not vaild, set to default.");
    Configuration::instance().set(HttpField::ROOT, DEFAULT_CFG);
    return DEFAULT_CFG;
}

HttpSession::HttpSession(QObject* parent)
    : NetworkSession(QML_MODULE_NAME, QML_MODULE_MAJOR, QML_MODULE_MINOR)
    , lock_() {
    QVariantMap cfg = atleastHttpField();

    if (cfg.contains(HttpField::BaseUrl)) {
        this->baseUrl_ = cfg[HttpField::BaseUrl].toString();
    }

    if (int userIdx = -1; copy(userIdx, cfg, HttpField::UserId)) {
        if (QVariantList list; copy(list, cfg, HttpField::Users)) {
            auto info = list.at(userIdx).value<QVariantMap>();
            copy(this->userId_, info, HttpField::UserId);
            copy(this->username_, info, HttpField::Username);
        }
    }

    network_ = new QNetworkAccessManager(this);
    network_->setRedirectPolicy(QNetworkRequest::RedirectPolicy::SameOriginRedirectPolicy);
    instance_ = this;
}

HttpSession::~HttpSession() {

}

bool HttpSession::load(QString& error) {
    if (!QNetworkInformation::loadDefaultBackend())
        qCritical() << "No availble network backend, network will not accessible.";
    connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this, &HttpSession::onlineStateChanged);
    auto props = atleastHttpField();
    if (int lastUserId = -1; copy(lastUserId, props, HttpField::LastUser)) {
        auto users = this->userLists_ = props.value(HttpField::Users).value<QVariantList>();
        auto user = users.at(lastUserId).value<QVariantMap>();
        int code = -1;
        if (QString username, userPassage;
            copy(username, user, HttpField::UserId) && 
            copy(userPassage, user, HttpField::UserPassage) && 
            !login(code, error, username, userPassage))
            qWarning() 
                << lstr("The startup login operation is failed, last user wasn't registered, message: %1").arg(error);
    }

    return true;
}

bool HttpSession::unload(QString&) {
    if (this->userId_.size())
        this->logout();
    // auto cfg = atleastHttpField();
    // Configuration::instance().set(HttpField::ROOT, qMove(cfg));
    return true;
}

void HttpSession::release() noexcept {
    delete this;
}

bool HttpSession::login(int& errorCode, QString& error, QString userId, QString passage) {
    userId = userId.trimmed(); passage = passage.trimmed();
    if (userId.isEmpty() || passage.isEmpty()) {
        errorCode = 450;
        error = lstr("用户和密码都不能为空，请输入对应信息！");
        return false;
    }
    if (this->username_.size()) {
        this->logout();
    }
    else {
        auto respone = this->post(HttpAddress::USER, RequestCode::Login, {
                {HttpField::DATA, QVariant(QJsonObject::fromVariantMap({
                    {HttpField::UserId, QVariant(userId)},
                    {HttpField::UserPassage, QVariant(passage)}
                }))}
            });
        if (respone)
            connect(respone, &NetworkRespone::responsed, this, [this, userId, passage](NetworkRespone* respone) {
            auto map = respone->get(false);
            int code = map[NetworkField::CODE].toInt();
            if (code == ResponseCode::Success) {
                QVariantMap data; copy(data, map, NetworkField::DATA);
                if (!copy(this->username_, data, HttpField::Username)) {
                    this->username_ = lstr("新建用户");
                    qWarning() << lstr("Get username failure, use default name:%1").arg(this->username_);
                }
                // if (QByteArray array; copy(array, data, HttpField::Authorization))
                //     this->authors_.fromUtf8(array);
                // else 
                //     qWarning() << lstr("Get authorization failure.");

                if (!copy(this->authors_, data, HttpField::Authorization))
                    qWarning() << lstr("Get authorization failure.");
                
                QVariantMap usrinfo {
                    {HttpField::UserId, QVariant(userId)},
                    {HttpField::UserPassage, QVariant(passage)},
                    {HttpField::Authorization, QVariant(this->authors_)},
                    {HttpField::Username, QVariant(this->username_)},
                };
                bool append = true;
                for (auto& usrv : this->userLists_) {
                    auto usr = usrv.value<QVariantMap>();
                    if (usr.value(HttpField::UserId) == userId) {
                        usrv = QVariant(usrinfo);
                        append = false;
                    }
                }
                if (append) 
                    this->userLists_.append(usrinfo);
                
                Configuration::instance().set(HttpField::USERS, this->userLists_);

                this->userId_ = userId;
                emit userNameChanged();
                emit userIdChanged();
            }
            else if (code == ResponseCode::Forbidden)
                emit errorOccured(code, lstr("登录失败，用户名或密码不正确。"));
            else
                emit errorOccured(code, lstr("登录失败，未知错误 %1 。").arg(code));
                }, Qt::SingleShotConnection);
        else {
            errorCode = 423;
            error = lstr("登录失败，请稍后再试。");
            return false;
        }
    }
    errorCode = ResponseCode::Success;
    error = ResponseString::Success;
    return true;
}

void HttpSession::login(QString userId, QString passage) {
    int errorCode = 0;
    QString error;
    if (!login(errorCode, error, userId, passage))
        emit errorOccured(errorCode, error);
}

void HttpSession::logout(int specialCode, QString userId, QString passage) {
    emit this->beforeLogout();
    auto respone = this->patch(HttpAddress::USER, RequestCode::Logout, {
            {HttpField::UserId, QVariant(this->userId_)},
            {HttpField::Username, QVariant(this->username_)}
        });
    if (respone) {
        userId = userId.trimmed();
        passage = passage.trimmed();
        connect(respone, &NetworkRespone::responsed, this, 
            [this, respone, userId, passage]() {
                auto map = respone->get(false);
                auto code = map[NetworkField::CODE].toInt();
                if (code == ResponseCode::Success) {
                    this->userId_ = {};
                    this->username_ = {};
                    emit userNameChanged();
                    emit userIdChanged();
                    if (userId.size() && passage.size())
                        this->login(userId, passage);
                } else
                    emit errorOccured(ResponseCode::InternalError, lstr("登出时信息同步失败，已撤销操作。"));
            }, Qt::SingleShotConnection);
    } else
        emit errorOccured(ResponseCode::InternalError, lstr("登出时信息同步失败，已撤销操作。"));
}

void HttpSession::logout() {
    this->logout(RequestCode::Logout, {}, {});
}

bool failHeader(QString& error, QString& name, QVariantMap map, int serialCode, QUrl const& url) {
    if (!map.contains(HttpField::HEADER_Type_Data)) {
        error = lstr("Request code %1 to '%2' customized header missing name, skipped.").arg(serialCode).arg(url.toString());
        return true;
    }
    name = map[HttpField::HEADER_Type_Data].toString();
    if (name.isEmpty()) {
        error = lstr("Request code %1 to '%2' customized header name empty, skipped.").arg(serialCode).arg(url.toString());
        return true;
    }
    return false;
}

template<typename T>
void appendHeaders(T& headers, QVariant headerObject, int serialCode, QUrl const& url) {
    if (headerObject.isValid() && headerObject.canConvert<QVariantList>()) {
        for (auto i{ 0u }; auto c : headerObject.value<QVariantList>()) {
            if (c.isValid() && c.metaType() == QMetaType::fromType<QVariantMap>()) {
                auto map = c.value<QVariantMap>();
                if (!map.contains(HttpField::HEADER_Type)) {
                    qWarning() << lstr("Request code %1 to '%2' at[%3] doesnt have header type, skipped.").arg(serialCode).arg(url.toString()).arg(i);
                    return;
                }

                if (!map.contains(HttpField::HEADER_Value)) {
                    qWarning() << lstr("Request code %1 to '%2' at[%3] doesnt have header value, skipped.").arg(serialCode).arg(url.toString()).arg(i);
                    return;
                }

                bool ok; auto type = map[HttpField::HEADER_Type].toInt(&ok);
                if (!ok) {
                    qWarning() << lstr("Request code %1 to '%2' at[%3] header type is value of '%3', which is not vaild, skipped.")
                        .arg(serialCode).arg(url.toString()).arg(map[HttpField::HEADER_Type].metaType().id()).arg(i);
                    return;
                }
                auto value = map[HttpField::HEADER_Value].value<QVariant>();
                if (!value.isValid() || !value.canConvert<QByteArray>()) {
                    qWarning() << lstr("Request code %1 to '%2' at[%3] header value is value of '%3', which is not vaild, skipped.")
                        .arg(serialCode).arg(url.toString()).arg(map[HttpField::HEADER_Type].metaType().id()).arg(i);
                    return;
                }

                switch (type) {
                case HttpField::HEADER_Type_KnownHeader: {
                    if (map.contains(HttpField::HEADER_Type_Data) &&
                        map[HttpField::HEADER_Type_Data].canConvert<int>())
                        headers.setHeader(static_cast<QNetworkRequest::KnownHeaders>(map[HttpField::HEADER_Type_Data].value<int>()), value);
                    else
                        qWarning() << lstr("Request code %1 to '%2' at[%3] header‘s data is not QNetworkRequest::KnownHeaders, skipped.")
                            .arg(serialCode).arg(url.toString()).arg(i);
                } break;
                                                       // QHttpPart doesnt have setHeaders.
                case HttpField::HEADER_Type_Http:
                    if constexpr (requires{ headers.setHeaders(::std::declval<QHttpHeaders>()); }) {
                        QString error, name;
                        if (failHeader(error, name, map, serialCode, url)) {
                            qWarning("%1", error);
                            break;
                        }

                        // value should be convertible to QByteArray or QString
                        auto httpHeaders = headers.headers();
                        httpHeaders.replaceOrAppend(name.toUtf8(), value.toByteArray());
                        headers.setHeaders(qMove(httpHeaders));
                    }
                    else
                        qWarning() << lstr("Request code %1 to '%2' at[%3] header type cannot recognized, skipped.")
                            .arg(serialCode).arg(url.toString()).arg(i);
                    break;
                case HttpField::HEADER_Type_Customized: {
                    QString error, name;
                    if (failHeader(error, name, map, serialCode, url)) {
                        qWarning("%1", error);
                        break;
                    }

                    headers.setRawHeader(name.toUtf8(), value.value<QByteArray>());
                } break;
                default:
                    qWarning() << lstr("Request code %1 to '%2' at[%3] header type cannot recognized, skipped.")
                        .arg(serialCode).arg(url.toString()).arg(i);
                    break;
                }
            }
            else
                qWarning() << lstr("Request code %1 to '%2' at[%3] value cannot recognized, skipped.")
                    .arg(serialCode).arg(url.toString()).arg(i);
            i++;
        }
    }
    else
        qWarning() << lstr("Request code %1 to '%2' value cannot recognized, skipped.")
            .arg(serialCode).arg(url.toString());
}

NetworkRespone* HttpSession::request(Network::Request operation, QString localUrl, int serialCode, QVariantMap payload) {
    return request(operation, QUrl(this->baseUrl_ + '/' + localUrl), serialCode, payload);
}

NetworkRespone* HttpSession::request(Network::Request operation, QUrl const& url, int serialCode, QVariantMap payload) {
    QByteArray sendBatch;
    bool requireData = true;
    switch (operation)
    {
    case Network::Request::Get:
        sendBatch = lstr("GET").toUtf8();
        requireData = false;
        break;
    case Network::Request::Post:
        sendBatch = lstr("POST").toUtf8();
        break;
    case Network::Request::Put:
        sendBatch = lstr("PUT").toUtf8();
        break;
    case Network::Request::Patch:
        sendBatch = lstr("PATCH").toUtf8();
        break;
    case Network::Request::Delete:
        sendBatch = lstr("DELETE").toUtf8();
        requireData = false;
        break;
    case Network::Request::Head:
        sendBatch = lstr("HEAD").toUtf8();
        requireData = false;
        break;
    default:
        qCritical("Encounting with unknown operation code %1, request ignored.", static_cast<int>(operation));
        return nullptr;
    }

    qDebug() << lstr("Send request %1 code %2 to %3, begin.").arg(sendBatch).arg(serialCode).arg(url.toString());

    QObject stub;
    QNetworkRequest request(url);
    if (payload.contains(HttpField::HEADER))
        appendHeaders(request, payload[HttpField::HEADER], serialCode, url);

    if (logined()) {
        request.setRawHeader(HttpField::Authorization.toUtf8(), this->authors_.toUtf8());
        
    }

    QNetworkReply* reply;
    QHttpMultiPart* sendMultipart = nullptr;
    QIODevice* sendIoDevice = nullptr;
    QByteArray sendContent;
    auto dataValue = payload.contains(HttpField::DATA) ? payload[HttpField::DATA] : payload;
    if (dataValue.canConvert<QIODevice*>()) {
        sendIoDevice = unbox<QIODevice>(dataValue);
        // reply = this->network_->sendCustomRequest(request, sendBatch, sendIoDevice);
        sendIoDevice->setParent(&stub);
    }
    else if (dataValue.canConvert<QByteArray>()) { // should utf8 string.
        sendContent = dataValue.value<QByteArray>();
        if (sendContent.isEmpty())
            qWarning() << lstr("Request code %1 to '%2' data content is empty, data will be ignored.").arg(serialCode).arg(url.toString());
    }
    else if (dataValue.canConvert<QVariantMap>()) {
        auto mmap = dataValue.value<QVariantMap>();

        int kind = HttpField::DATA_Kind_Json;
        copy(kind, mmap, HttpField::DATA_Kind);
        if (kind < 0 && kind > HttpField::DATA_Kind_Json) {
            qCritical() << lstr("Request code %1 to '%2' multi data kind unknown, ignored.").arg(serialCode).arg(url.toString());
            return nullptr;
        }

        if (kind != HttpField::DATA_Kind_Json) {
            sendMultipart = new QHttpMultiPart(QHttpMultiPart::ContentType(kind - 1));
            for (auto i = 0u; auto v : mmap[HttpField::DATA].value<QVariantList>()) {
                if (v.isValid() && v.canConvert<QVariantMap>()) {
                    QHttpPart part;
                    auto httpMap = v.value<QVariantMap>();
                    // append headers.
                    if (httpMap.contains(HttpField::MULTIDATA_Header))
                        if (httpMap[HttpField::MULTIDATA_Header].isValid())
                            appendHeaders(part, httpMap[HttpField::MULTIDATA_Header], serialCode, url);
                        else
                            qWarning() << lstr("Http multi object at[%1] have empty header, skipped.").arg(i);
                    if (httpMap.contains(HttpField::MULTIDATA_Data)) {
                        auto v = httpMap[HttpField::MULTIDATA_Data];
                        if (v.canConvert<QByteArray>())
                            part.setBody(v.value<QByteArray>());
                        else if (v.canConvert<QIODevice*>())
                            part.setBodyDevice(unbox<QIODevice>(v, sendMultipart));
                        else
                            qWarning() << lstr("Http multi object at[%1] have invaild object, skipped.").arg(i);
                    }
                    sendMultipart->append(part);
                }
                else
                    qCritical() << lstr("Request %4 code %1 to '%2' data[%3] cannot be decoded, request ignored.")
                        .arg(serialCode).arg(url.toString()).arg(i).arg(sendBatch);
                i++;
            }
        }
        else
            sendContent = QJsonDocument::fromVariant(mmap).toJson();
    }
    else
        qWarning("Request %3 code %1 to '%2' data cannot identified, data will be ignored.",
            serialCode, url.toString(), sendBatch);

    if (sendMultipart || sendIoDevice || !sendContent.isEmpty()) {
        auto haveProgress = false;
        if (sendMultipart) {
            reply = this->network_->sendCustomRequest(request, sendBatch, sendMultipart);
            sendMultipart->setParent(reply);
        }
        else if (sendIoDevice) {
            reply = this->network_->sendCustomRequest(request, sendBatch, sendIoDevice);
            sendIoDevice->setParent(reply);
            haveProgress = true;
        }
        else if (sendContent.size()) {
            reply = this->network_->sendCustomRequest(request, sendBatch, sendContent);
        }
        else if (!requireData) {
            reply = this->network_->sendCustomRequest(request, sendBatch);
        }
        else {
            qWarning() << lstr("Request code %1 to '%2' required data but not given, ignored operation.").arg(serialCode).arg(url.toString());
            return nullptr;
        }

        {
            QWriteLocker locker(&this->lock_);
            if (auto it = this->idToReply_.find(serialCode); it != this->idToReply_.end()) {
                auto rreply = it.value();
                rreply->abort();
                rreply->deleteLater();
                qWarning() << lstr("Request code %1 to '%2' have multiple request, abort previous.").arg(serialCode).arg(url.toString());
                it.value() = reply;
            }
            else
                this->idToReply_.insert(serialCode, reply);
        }

        auto respone = new HttpResponse(reply, this, serialCode, -1);
        reply->setProperty(PROPERTY_RESPONE, box(respone));
        connect(reply, &QNetworkReply::finished, respone, &HttpResponse::handleFinished, Qt::SingleShotConnection);
        if (haveProgress) {
            connect(reply, &QNetworkReply::uploadProgress, respone, &HttpResponse::handleProgress, Qt::SingleShotConnection);
            connect(reply, &QNetworkReply::downloadProgress, respone, &HttpResponse::handleProgress, Qt::SingleShotConnection);
        }
        return respone;
    }
    else
        return nullptr;
}

void HttpSession::setUsername(QString username) {
    username = username.trimmed();
    if (username.isEmpty()) {
        emit errorOccured(450, lstr("用户名不能为空，请输入对应信息！"));
        return;
    }
    if (username == this->username_)
        return;
    if (this->userId_.isEmpty()) {
        emit errorOccured(ResponseCode::Forbidden, lstr("请先登录后再修改用户名。"));
        return;
    }

    auto respone = this->patch(HttpAddress::USER, RequestCode::SetUsername, {
            {HttpField::UserId, QVariant(this->userId_)},
            {HttpField::Username, QVariant(username)}
        });
    if (respone)
        connect(respone, &NetworkRespone::responsed, this, [this, username](NetworkRespone* respone) {
        auto map = respone->get(false);
        auto code = map[NetworkField::CODE].toInt();
        if (code == ResponseCode::Success) {
            this->username_ = username;
            emit userNameChanged();
        }
        else
            emit errorOccured(code, lstr("修改用户名失败，未知错误。"));
            }, Qt::SingleShotConnection);
    else
        emit errorOccured(ResponseCode::InternalError, lstr("修改用户名失败，请稍后再试。"));
}

void HttpSession::create(QString name, QString passage) {
    name = name.trimmed(); passage = passage.trimmed();
    if (name.isEmpty() || passage.isEmpty()) {
        emit errorOccured(450, lstr("用户名和密码都不能为空，请输入对应信息！"));
        return;
    }

    auto respone = this->post(HttpAddress::USER, RequestCode::CreateUser, {
            {HttpField::Username, QVariant(name)},
            {HttpField::UserPassage, QVariant(passage)}
        });
    if (respone)
        connect(respone, &NetworkRespone::responsed, this, [this, name](NetworkRespone* response) {
        auto map = response->get(false);
        auto code = map[NetworkField::CODE].toInt();
        if (QString userId; code == ResponseCode::Success &&
            copy(userId, map, HttpField::UserId)) {
            this->userId_ = userId;
            this->username_ = lstr("新建用户%1").arg(userId);
            emit userNameChanged();
            emit userIdChanged();
        }
        else
            emit errorOccured(code, lstr("创建用户失败，未知错误。"));
            }, Qt::SingleShotConnection);
    else
        emit errorOccured(ResponseCode::InternalError, lstr("创建用户失败，请稍后再试。"));
}

void HttpSession::deleteUser() {
    if (this->userId_.isEmpty()) {
        emit errorOccured(ResponseCode::Forbidden, lstr("请先登录后再删除用户。"));
        return;
    }

    // 修正未定义的 NetworkAddress::User 为统一的 HttpAddress::USER
    auto respone = this->request(Network::Request::Delete,
        HttpAddress::USER + lstr("/") + userId_, RequestCode::DeleteUser);
    if (respone)
        connect(respone, &NetworkRespone::responsed, this, [this](NetworkRespone* respone) {
        auto map = respone->get(false);
        auto code = map[NetworkField::CODE].toInt();
        if (code == ResponseCode::Success) {
            this->userId_ = {};
            this->username_ = {};
            emit userNameChanged();
            emit userIdChanged();
        }
        else
            emit errorOccured(code, lstr("删除用户失败，未知错误。"));
            }, Qt::SingleShotConnection);
    else
        emit errorOccured(ResponseCode::InternalError, lstr("删除用户失败，请稍后再试。"));
}

MODULE_ENTRY(Q_DECL_EXPORT)() {
    return new HttpSession();
}