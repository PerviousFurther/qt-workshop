#pragma once

#include "Session.hpp"

#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkInformation>
#include <QReadWriteLock>
#include <QUrl>
#include <QVariantMap>
#include <QString>

class HttpResponse;
class HttpSession : public NetworkSession {
	Q_OBJECT;
public:
	explicit HttpSession(QObject* parent = nullptr);
	~HttpSession() override;

	bool load(QString& error) override;
	bool unload(QString& error) override;

	void login(QString userId, QString passage) override;
	void logout() override;
	NetworkRespone* request(Network::Request operation, QUrl const& url, int serialCode = -1, QVariantMap payload = {}) override;
	NetworkRespone* request(Network::Request operation, QString url, int serialCode = -1, QVariantMap payload = {}) override;

	QString userId() override { return userId_; }
	QString userName() override { return username_; }

	void setUsername(QString username) override;
	void create(QString name, QString passage) override;
	void deleteUser() override;

	bool cloudEnabled() override { return cloudEnabled_; }
	bool logined() override { return cloudEnabled_ && !userId_.isEmpty(); }
	bool online() override { return QNetworkInformation::instance()->reachability() == QNetworkInformation::Reachability::Online; }

	void release() noexcept override;

public slots:
	void logout(int specialCode, QString nextUserId, QString nextPassage);

	bool login(int& errorCode, QString& error, QString userId, QString passage);
	// void handleLogin(HttpResponse* response, QString id, QString passage);

private:
	mutable QReadWriteLock lock_;
	QNetworkAccessManager* network_ = nullptr;
	// QList<int> idOrds_;
	QHash<int, QNetworkReply*> idToReply_;
	bool cloudEnabled_ = false;
	QString userId_;
	QString username_;
	QString authors_;
	QString baseUrl_;

	QVariantMap headers_;
	QVariantList userLists_;
};