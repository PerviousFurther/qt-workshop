#pragma once

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QVariant>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QResizeEvent>
#include <QSizePolicy>

#include "Extensible.hpp"

class QListWidget;

class PageUser : public Extensible {
    Q_OBJECT
public:
    explicit PageUser(QWidget* parent = nullptr);
    ~PageUser() = default;

signals:
    void requestConnection();

public:
    qsizetype appendWidget(QWidget* widget) override;
    QWidget* widget(qsizetype index, QWidget* widget) override;
    QWidget* removeWidget(qsizetype index) noexcept override;

public slots:
    void updateStyle();
    void updateLoginState(bool loggedIn);
    void refreshContent();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void handleLogin();
    void onLoginStatusChanged();
    void onLoginError(int error, QString message);
    void handleLogout();
    void handleRegister();
    void showLoginPanel();
    void showRegisterPanel();

private:
    void adjustPageWidth();
    void updateAuthPanelVisibility();

private:
    QWidget* headerArea_{ nullptr };
    QWidget* contentArea_{ nullptr };

    QLabel* avatarLabel_{ nullptr };
    QLabel* nameLabel_{ nullptr };
    QLabel* descLabel_{ nullptr };
    QListWidget* profileList_{ nullptr };
    QPushButton* setBtn_{ nullptr };
    QWidget* loginContainer_{ nullptr };
    QWidget* loginPanelWidget_{ nullptr };
    QLabel* loginHintLabel_{ nullptr };
    QLineEdit* usernameEdit_{ nullptr };
    QLineEdit* passwordEdit_{ nullptr };
    QPushButton* loginBtn_{ nullptr };
    QPushButton* logoutBtn_{ nullptr };

    QWidget* promoWidget_{ nullptr };
    QWidget* promoPanelWidget_{ nullptr };
    QWidget* contentBoxWidget_{ nullptr };
    QHBoxLayout* contentHLayout_{ nullptr };
    QWidget* emptyContentWidget_{ nullptr };
    QWidget* emptyPanelWidget_{ nullptr };
    QLabel* promoTitleLabel_{ nullptr };
    QLabel* promoDescLabel_{ nullptr };
    QLabel* emptyTextLabel_{ nullptr };
    QLabel* emptySubTextLabel_{ nullptr };
    QPushButton* mockConnectBtn_{ nullptr };

    QWidget* registerPanelWidget_{ nullptr };
    QWidget* registerContainer_{ nullptr };
    QLabel* registerHintLabel_{ nullptr };
    QLineEdit* regUsernameEdit_{ nullptr };
    QLineEdit* regEmailEdit_{ nullptr };
    QLineEdit* regPasswordEdit_{ nullptr };
    QLineEdit* regConfirmEdit_{ nullptr };
    QPushButton* registerBtn_{ nullptr };
    QPushButton* gotoLoginBtn_{ nullptr };

    QPushButton* gotoRegisterBtn_{ nullptr };

    bool isRegisterMode_{ false };
    bool isLoggedIn_{ false };
};