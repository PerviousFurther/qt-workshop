#pragma once
#include <QHBoxLayout>
#include "Extensible.hpp"

class ConversationListPanel;
class ChatAreaPanel;
class ModelSettingsPanel;
enum ChatDisplayState {
    ChatLeftState,
    ChatCenteredState,
    ChatRightState
};
class PageChat : public Extensible {
    Q_OBJECT;
public:
    explicit PageChat(QWidget* parent = nullptr);
    virtual ~PageChat() override = default;

private slots:
    void handleLeft();
    void handleCenter();
    void handleRight();
    void updateTopTheme();

private:
    void transitionToState(int newState);

    QHBoxLayout* mainLayout_ = nullptr;
    ConversationListPanel* listPanel_ = nullptr;
    ChatAreaPanel* chatPanel_ = nullptr;
    ModelSettingsPanel* modelPanel_ = nullptr;

    int currentState = ChatDisplayState::ChatCenteredState;
};