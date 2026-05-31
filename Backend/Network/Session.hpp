#pragma once

#include "Core/Module.hpp"
#include <QString>
#include <QUrl>
#include <QSemaphore>
#include <QVariant>

class QUrl;
class QNetworkReply;

// Implementation can extend shared respone code in this namespace.

namespace ResponseCode {
    using Type = int;
    // the sucess code might not contain message.
    inline constexpr Type Success = 200;
    inline constexpr Type BadRequest = 400;
    inline constexpr Type InternalError = 500;
    inline constexpr Type NotFound = 404;
    inline constexpr Type Unauthorized = 401;
    inline constexpr Type Forbidden = 403;
    inline constexpr Type ClientOvertime = 408;
    inline constexpr Type Unknown = 450;
}

namespace Network {

    Q_NAMESPACE;
    
    // Get, Post, Put and eg. are the basic request types.
    // Download and Upload are for follow-up binary flows.
    // The implmentation should handle for stream operation.
    enum class Request {
        Get = 0,
        Post = 1,
        Put = 2,
        Patch = 3,
        Delete = 10,
        Head = 11,
    };

    Q_ENUM_NS(Request);
}

namespace NetworkField {
    inline constexpr auto MODULE_NAME = "Network";
}

// Network respone.
class
#if defined(RINGAPP_BACKEND_NETWORK_EXPORT)
Q_DECL_EXPORT
#else
Q_DECL_IMPORT
#endif
NetworkRespone : public QObject {
    Q_OBJECT;
    // QML_ELEMENT;
protected:
    struct Getter {
        struct iterator {
            iterator& operator++() noexcept { return *this; }
            QVariantMap operator*() const { return ref_->get(false); }
            
            bool operator!=(iterator) const noexcept { return ref_->endOfRequest(); }

            NetworkRespone* ref_;
        };

        iterator begin() const noexcept { return { ref_ }; }
        iterator end() const noexcept { return { ref_ }; }
        
        auto error() const noexcept { return ref_->error(); }
        auto code() const noexcept { return ref_->code(); }

        QVariantMap operator*() const { return ref_->get(true); }

        NetworkRespone* ref_;
    };

    // struct Setter {
    //     struct exposure {
    //         void operator=(QVariantMap map) const { ref_->set(map); }
    //         bool stop(QString& error) const { return ref_->stop(error); }
    //         QVariantMap get() const { return ref_->get(false); }
    // 
    //         NetworkRespone* ref_;
    //     };
    // 
    //     struct iterator {
    //         iterator& operator++() noexcept { return *this; }
    //         exposure operator*() const { return {ref_}; }
    // 
    //         bool operator!=(iterator) const noexcept { return ref_->endOfRequest(); }
    // 
    //         NetworkRespone* ref_;
    //     };
    //     
    //     auto error() const noexcept { return ref_->error(); }
    //     auto code() const noexcept { return ref_->code(); }
    // 
    //     iterator begin() const noexcept { return { ref_ }; }
    //     iterator end() const noexcept { return { ref_ }; }
    // 
    //     NetworkRespone* ref_;
    // };

protected:
    using QObject::QObject;

public:
    Q_PROPERTY(int code READ code NOTIFY responsed);
    Q_PROPERTY(int serialCode READ serialCode NOTIFY DUCK);
    Q_PROPERTY(QString error READ error NOTIFY responsed);

signals:
    void DUCK();
    // The signal is respone for get operation. call get if the timeout or some other things. 
    void responsed(NetworkRespone* response);
    void progressed(float percentage);

public:
    Getter get() { return {this}; }
    // Setter set() { return {this}; }

    virtual bool endOfRequest() const noexcept = 0;

    // error code.
    virtual int code() const noexcept = 0;
    virtual int serialCode() const noexcept = 0;
    // error message.
    virtual QString error() const noexcept = 0;

    virtual bool stop(QString& error) = 0;

    virtual QVariantMap get(bool waitEnd) = 0;
    // virtual void set(QVariantMap map) = 0;
};

// In design, other Session will reference this Session for interaction with Cloud side data.
//
// Do not offer concrete interface like `isXxxSync`. It broke the encapsulation of interface.
// Other Session should not be aware about it.
// DO NOT motify this class.
//
// Implementation should have constructor like (QObject*, QJsonDocument)
// Which first is parent of this object (usually the BackendModule)
// Second is the json properties inside configuration.
//
// Do not modify the interface.
class 
#if defined(RINGAPP_BACKEND_NETWORK_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
NetworkSession : public Module {
    Q_OBJECT;
public:
    Q_PROPERTY(QString userId READ userId NOTIFY userIdChanged);
    Q_PROPERTY(QString userName READ userName WRITE setUsername NOTIFY userNameChanged);
    Q_PROPERTY(bool online READ online NOTIFY onlineStateChanged);
    Q_PROPERTY(bool logined READ logined NOTIFY userIdChanged);
    Q_PROPERTY(bool cloudEnabled READ cloudEnabled NOTIFY cloudStateChanged);

    static QString moduleName();

protected:
    using Module::Module;

signals:
    // the signal is for UI, this operation only trigger when login, logout and setUsername.
    // Other operation should listening on response.
    void errorOccured(int error, QString message);
    
    void onlineStateChanged(); // network changed.
    // userId Changed means the user is changed, user can update user information here.
    void userIdChanged();
    // userId Changed not means the user is changed, since username can change easily.
    void userNameChanged();

    void cloudStateChanged();

    // Implmentation should make comment on own header file,
    // To notify invoker what will the `result` exporess, since we use VariantList.

    // to avoid trigger multiple time, user might listen on one of this function or NetworkResponse::responed.

    void responsed(NetworkRespone* result);
    void progressed(NetworkRespone* result, float percentage);
    
    void beforeLogout();

public:
    static auto instance() { return instance_; }
    // Username should get from server, and sync the data to local configuration.

    virtual void create(QString userId, QString passage) = 0;
    virtual void login(QString userId, QString passage) = 0;
    virtual void logout() = 0;
    virtual void setUsername(QString username) = 0;
    virtual void deleteUser() = 0;

    // Payload can contain a top-level header object.
    // Implementation should send payload.header as HTTP headers instead of request body.

    // url is just absolute. the header will automatically include user infomation and some known headers.
    virtual NetworkRespone* request(Network::Request operation, QUrl const& url, int requestSerialCode, QVariantMap payload = {}) = 0;
    // the localUrl will use as <baseUrl>/<localUrl>. the header will automatically include user infomation and some known headers.
    virtual NetworkRespone* request(Network::Request operation, QString localUrl, int requestSerialCode, QVariantMap payload = {}) = 0;
    // the network is enable or not.
    virtual bool online() = 0;
    // the user is empty or not.
    virtual bool logined() = 0;
    virtual bool cloudEnabled() = 0;

    virtual QString userId() = 0;
    virtual QString userName() = 0;

#define RINGAPP_TRANSFER_DEFINE_(name, Name)\
    auto name(QString url, int requestSerialCode, QVariantMap payload = {}) {\
        return request(Network::Request::Name, url, requestSerialCode, payload);\
    }\
    auto name(QUrl const& url, int requestSerialCode, QVariantMap payload = {}) { \
            return request(Network::Request::Name, url, requestSerialCode, payload); \
    }
    RINGAPP_TRANSFER_DEFINE_(get, Get);
    RINGAPP_TRANSFER_DEFINE_(put, Put);
    RINGAPP_TRANSFER_DEFINE_(post, Post);
    RINGAPP_TRANSFER_DEFINE_(head, Head);
    RINGAPP_TRANSFER_DEFINE_(patch, Patch);
#undef RINGAPP_TRANSFER_DEFINE_

protected:
    inline static NetworkSession* instance_ = nullptr;
};

namespace RequestCode {
    using Type = int;
    inline constexpr Type UserCodeStart = 0x70000000;
}

namespace NetworkField {
    // if the response is Json map, then it must contain the key.
    inline const auto CODE = lstr("code");
    // if the response is Json map, then it must contain the key.
    inline const auto MSG = lstr("msg");
    // if the response is Json map, then it might contain the key.
    inline const auto DATA = lstr("data");

    // it needs QIODevice*.
    inline const auto SET_Kind_IoDevice = lstr("^d");
    // if json, it needs utf-8.
    inline const auto SET_Kind_Bytes = lstr("^b");
    // use for respone to set, requires DATA also. If no data is set, then consider can end o
    inline const auto SET_Kind = lstr("^sk");

}

namespace NetworkAddress {
    template<typename T, typename...Ts>
    QString address(T base, Ts...value) {
        return QString(base) + (('/' + QString(value)) + ...);
    }
    // QString subdir(QString address);

    inline const auto User = lstr("user");
    inline const auto Device = lstr("device");
}

