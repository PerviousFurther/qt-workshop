#pragma once

#include <QStringList>
#include <QJsonObject>
#include <QVariantMap>

#include "Session.hpp"

class QString;
class AiChatSessionImpl : public AiChatSession {
    Q_OBJECT
public:
    explicit AiChatSessionImpl();
    ~AiChatSessionImpl() override;
    
    bool load(QString&) override;
    bool unload(QString&) override;
    void release() noexcept override;

    void switchConversation(int index = -1) override;
    void sendMessage(QJsonObject const& context, int requestCode = -1) override;
    void switchModel(QString const& modelName) override;
    QStringList availbleModel() override;

private slots:
    // 监听网络会话的用户变更及在线状态，用于触发同步
    void handleUserOrNetworkChanged();

private:
    // 从云端同步模型列表 (对应 Server/ai/query.json)
    void syncCloudModels();

    // 同步本地配置的聊天历史 (对应 Local/config.json)
    void syncLocalConfig();

private:
    QStringList m_availableModels;
    QString m_currentModel;
    int m_currentConversationIndex = -1;
    QString m_lastSyncedUserId;
};