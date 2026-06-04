#include "Core/Module.hpp"

class SleepDashboardWidget;
class 
#if defined(RINGAPP_TOOL_SLEEP_EXPORT)
	Q_DECL_EXPORT
#else
	Q_DECL_IMPORT
#endif
ToolSleep : public Module {
	Q_OBJECT;
public:
	ToolSleep();
	~ToolSleep();
public:
	Q_PROPERTY(QStringList devices READ devices NOTIFY deviceChanged);

signals:
	void deviceChanged();

public:
	bool load(QString&) override;
	bool unload(QString&) override;
	void release() noexcept override;

	QStringList devices() const noexcept { return availbleDevices_; }

private slots:
	void onConnected(int id);
	void onConnectionError(int id, int, QString);

private:
	QStringList availbleDevices_;
	QList<qsizetype> cardIds_;
	SleepDashboardWidget* detailCards_ = nullptr;
	QWidget* pageSleep_ = nullptr;
};


