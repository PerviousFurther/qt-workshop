#include <QDebug>
#include <QJsonDocument>
// #include <QJson>
#include "Backend/Network/Session.hpp"
#include "SessionPrivate.hpp"

AiChatSessionImpl::AiChatSessionImpl()
    : AiChatSession(QML_MODULE_NAME, QML_MODULE_MAJOR, QML_MODULE_MINOR)
{
    Q_ASSERT_X(!instance_, "AiChatSession::AiChatSession", "Chat session can only initialize once.");

    auto* session = NetworkSession::instance();
    if (session) {
        connect(session, &NetworkSession::userIdChanged, this, &AiChatSessionImpl::handleUserOrNetworkChanged);
        connect(session, &NetworkSession::onlineStateChanged, this, &AiChatSessionImpl::handleUserOrNetworkChanged);
    }

    instance_ = this;

    this->handleUserOrNetworkChanged();
}

AiChatSessionImpl::~AiChatSessionImpl() {
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

bool AiChatSessionImpl::load(QString&) {
    return true;
}

bool AiChatSessionImpl::unload(QString&) {
    return true;
}

void AiChatSessionImpl::release() noexcept {
    delete this;
}

void AiChatSessionImpl::handleUserOrNetworkChanged() {
    auto* session = NetworkSession::instance();
    if (!session) return;

    QString currentUserId = session->userId();
    if (!currentUserId.isEmpty()) {
        if (currentUserId != m_lastSyncedUserId) {
            m_lastSyncedUserId = currentUserId;
            syncLocalConfig();
        }
        if (session->online()) {
            syncCloudModels();
        }
    } else {
        m_lastSyncedUserId.clear();
        m_availableModels.clear();
        emit availbleModelChanged();
    }
}

void AiChatSessionImpl::syncLocalConfig()
{
    qDebug() << "[AiChatSession] Syncing local config for user:" << m_lastSyncedUserId;
}

void AiChatSessionImpl::syncCloudModels()
{
    auto* session = NetworkSession::instance();
    if (!session) return;

    NetworkRespone* response = session->get("ai/query", RequestCode::UserCodeStart + 100);
    if (!response) { 
        qInfo() << "[AiChatSession ERROR] Sync cloud failure.";
        return; 
    }

    connect(response, &NetworkRespone::responsed, this, [this, response]() {
        if (response->code() == ResponseCode::Success) {
            QVariantMap resMap = response->get(false);
            QVariantMap dataField = resMap.value(NetworkField::DATA).toMap();
            m_availableModels = dataField.value("models").toStringList();
            emit availbleModelChanged();
        } else {
            qWarning() << "[AiChatSession] Failed to sync models:" << response->error();
        }
        response->deleteLater();
    });
}

void AiChatSessionImpl::switchConversation(int index)
{
    m_currentConversationIndex = index;
    // 触发基类定义的信号
    emit conversationSwitched(m_currentConversationIndex);
    qDebug() << "[AiChatSession] Switched to conversation index:" << index;
}

void AiChatSessionImpl::switchModel(QString const& modelName)
{
    if (m_currentModel != modelName) {
        m_currentModel = modelName;
        emit modelSwitched(m_currentModel);
    }
}

QStringList AiChatSessionImpl::availbleModel() {
    return m_availableModels;
}

void AiChatSessionImpl::sendMessage(QJsonObject const& context, int requestCode)
{
    auto* session = NetworkSession::instance();
    if (!session || !session->logined()) {
        emit responeError(requestCode, ResponseCode::Unauthorized, "User not logged in.");
        return;
    }

    // 构造请求载荷 payload 
    QVariantMap payload;
    payload["model"] = m_currentModel;
    payload["context"] = context.toVariantMap(); // 将标准 QJsonObject 转化为 QVariantMap
    payload["conversationIndex"] = m_currentConversationIndex;

    // 假设发送对话消息到 AI 服务的地址为 "ai/chat"
    // 注意：此处严格遵守了你的头文件拼写：用的是 `responsed` 信号
    NetworkRespone* response = session->post("ai/chat", requestCode, payload);
    if (!response) {
        emit responeError(requestCode, ResponseCode::InternalError, "Network library failed to create request.");
        return;
    }

    // 处理网络返回
    connect(response, &NetworkRespone::responsed, this, [this, requestCode, response]() {
        QVariantMap resMap = response->get(false);
        int httpCode = response->code();

        if (httpCode == ResponseCode::Success) {
            // 根据 NetworkField 规范解析基础结构
            // int serverCode = resMap.value(NetworkField::CODE).toInt();
            // QString msg = resMap.value(NetworkField::MSG).toString();
            QVariantMap dataField = resMap.value(NetworkField::DATA).toMap();

            QString replyMessage = dataField.value("reply").toString();
            QString modelUsed = dataField.value("model").toString();
            if (modelUsed.isEmpty()) modelUsed = m_currentModel;

            // 成功接收，分发消息信号
            emit messageReceive(requestCode, modelUsed, replyMessage);
        }
        else {
            // 失败流，分发错误信号
            QString errStr = response->error();
            if (errStr.isEmpty()) {
                errStr = resMap.value(NetworkField::MSG).toString();
            }
            emit responeError(requestCode, httpCode, errStr);
        }

        // 延迟释放 NetworkRespone 实例，防止内存泄漏
        response->deleteLater();
        });

    // 可选：如果前端需要进度条，转发百分比进度
    connect(response, &NetworkRespone::progressed, this, [this, requestCode](float percentage) {
        // 如果基类有扩展需求，可以在此处转发进度
        });
}

MODULE_ENTRY(Q_DECL_EXPORT)() {
    return new AiChatSessionImpl();
}