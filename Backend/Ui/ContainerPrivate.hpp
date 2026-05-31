#pragma once

#include <QListWidget>

#include "Container.hpp"

class SidebarItem : public QListWidgetItem {
public:
	SidebarItem(QListWidget* parent, int index, QString title, QString iconTxt, QUrl iconUrl, QUrl backgroundUrl);
private:
	// QAbstractAnimation* amim_ = nullptr;
};