#pragma once

#include "PageAiChat.hpp"

class QPushButton;
class QListWidget;
class QLineEdit;


class VerticalTitleBarAi : public QWidget {
    Q_OBJECT;
signals:
    void clicked();
public:
    VerticalTitleBarAi(const QString& text, QWidget* parent = nullptr);
    void setLabelStyle(const QString& style) { if (label_) label_->setStyleSheet(style); }
protected:
    void mousePressEvent(QMouseEvent* event) override { emit clicked(); }
private:
    QLabel* label_ = nullptr;
};

// ====================================
// 左面板：对话历史列表
// ====================================
class ConversationListPanel : public QWidget {
    Q_OBJECT;
signals:
    void titleClicked();
public:
    ConversationListPanel(QWidget* parent = nullptr);

    void setExpanded(bool expanded);

private:
    void updateThemeAndStyles();

    VerticalTitleBarAi* titleBar_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    QPushButton* newChatBtn_ = nullptr;
    QListWidget* historyList_ = nullptr;
};

// ====================================
// 中面板：主对话交互区
// ====================================
class ChatAreaPanel : public QWidget {
    Q_OBJECT;
signals:
    void titleClicked();
public:
    ChatAreaPanel(QWidget* parent = nullptr);

    void setExpanded(bool expanded);

private slots:
    void onSendTriggered();

    void onMessageReceived(int requestCode, QString modelName, QString message);

    void onResponseError(int requestCode, int errorCode, QString msg);

private:
    void appendMessageItem(const QString& sender, const QString& text, bool isSelf);

    void updateThemeAndStyles();

    VerticalTitleBarAi* titleBar_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    QListWidget* chatViewer_ = nullptr;
    QLineEdit* inputLine_ = nullptr;
    QPushButton* sendBtn_ = nullptr;
};

// ====================================
// 右面板：模型设置面板
// ====================================
class ModelSettingsPanel : public QWidget {
    Q_OBJECT;
signals:
    void titleClicked();
public:
    ModelSettingsPanel(QWidget* parent = nullptr);

    void setExpanded(bool expanded);

private:
    void refreshModels();

    void updateThemeAndStyles();

    VerticalTitleBarAi* titleBar_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    QListWidget* modelList_ = nullptr;
};

