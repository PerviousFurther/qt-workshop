#pragma once

#include "Backend/Ui/Extensible.hpp"

class QScrollArea;
class QVBoxLayout;
class QPushButton;
class QLabel;
class UiTestDetailPage : public Extensible {
	Q_OBJECT;
public:
	UiTestDetailPage(int id, QString title, QWidget* parent = nullptr);

signals:
	void measurementFinished(bool success);
	void requestMeasure(int deviceId);
	void requestBack();

public slots:
	void onMeasurementResultReceived(bool success);

public:
	qsizetype appendWidget(QWidget* widget) override;
	QWidget* removeWidget(qsizetype id) noexcept override;

private slots:
	void collectAndSendBytes();
	void updateStyles();

private:
	int deviceId_;

	QScrollArea* scrollArea_ = nullptr;
	QWidget* scrollContent_ = nullptr;
	QVBoxLayout* scrollLayout_ = nullptr;
	QPushButton* sendButton_ = nullptr;
	QPushButton* backButton_ = nullptr;
	QLabel* titleLabel_ = nullptr;
};