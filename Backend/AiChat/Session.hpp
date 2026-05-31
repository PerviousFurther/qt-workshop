#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "Core/Module.hpp"

// Implmentation should noticed.
//
// When chatSession initialize, and the cloudSession.userId is not empty. sync configuration's chat if the userid is match.
//  example see `Local/config.json`.
// it needs to call cloudSession.get to sync with cloud's model also in this time, btw. 
//  respone example see `Server/ai/query.json`.
// 
// Implmentation should use cloudSession to send message to llm server.
class 
#if defined(RINGAPP_BACKEND_AICHAT_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
AiChatSession : public Module {
    Q_OBJECT;
public:
    static constexpr auto name {"AiChatSession"};
    static constexpr auto moduleName {"Backend"};
    
    Q_PROPERTY(QStringList availbleModel READ availbleModel NOTIFY availbleModelChanged);

    using Module::Module;

signals:
    void availbleModelChanged();
    void conversationSwitched(int index);
    void messageReceive(int requestCode, QString modelName, QString message);
    void responeError(int requestCode, int errorCode, QString msg);
    void modelSwitched(QString const& modelName);

public:
    static AiChatSession* instance() noexcept { return instance_; }
    // push -1 to append new coversation.
    virtual void switchConversation(int index = -1) = 0;
    // requestCode is -1 will ignore it.
    // Some llm might allow push file or something, use json here.
    virtual void sendMessage(QJsonObject const& context, int requestCode = -1) = 0;
    virtual void switchModel(QString const& modelName) = 0;

    virtual QStringList availbleModel() = 0;

protected:
    inline static AiChatSession* instance_ = nullptr;
};