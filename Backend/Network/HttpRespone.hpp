#pragma once

#include "Session.hpp"
#include <QNetworkReply>
#include <QMutex>
#include <QWaitCondition>

class HttpSession;
class HttpResponse : public NetworkRespone {
	Q_OBJECT;
public:
	// deadtimeDelay is in msecs
	explicit HttpResponse(QNetworkReply* parent, HttpSession* session, int serialCode, int deadtimeDelay = -1, bool read = false);
	~HttpResponse() override;

signals:
	void finished(HttpResponse* respone);

public:
	QVariantMap get(bool waitEnd = false) override;
	// void set(QVariantMap info) override;

	bool stop(QString& error) override;

	int code() const noexcept override { return code_; }
	int serialCode() const noexcept override { return serialCode_; }
	QString error() const noexcept override { return error_; }

	void code(int value) noexcept { code_ = value; }
	void error(QString value) noexcept { error_ = value; }

	bool endOfRequest() const noexcept override;

	QNetworkReply* parent() const noexcept;

public slots:
	void handleProgress(qint64 current, qint64 total);
	void handleFinished();
	/*void handleWritten();*/
	void handleError(QNetworkReply::NetworkError);

private:
	int serialCode_;
	int code_{ -1 };
	int deadTime_{ -1 };
	int type_{ 0 };

	qint64 currentSize_ = -1;
	qint64 totalSize_ = -1;

	mutable QRecursiveMutex mtx_{};
	QString error_{};
	HttpSession* sess_;
};
