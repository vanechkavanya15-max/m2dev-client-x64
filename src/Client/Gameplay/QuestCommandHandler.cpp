#include "QuestCommandHandler.h"
#include "Client/Network/Protocol/Protocol.h" // We need CG::SCRIPT_ANSWER and CG::QUEST_CONFIRM
#include <EterBase/ModernLogger.h>
#include <vector>
#include <cstring>

namespace Client::Gameplay {

QuestCommandHandler::QuestCommandHandler(std::shared_ptr<Client::Core::INetworkPort> networkPort)
    : m_networkPort(std::move(networkPort))
{
}

Client::Core::Result<void, Client::Core::CommandError> QuestCommandHandler::Execute(const Client::Core::QuestAnswerCommand& cmd) {
    if (!m_activeScriptDialog) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }
    if (cmd.answerIndex >= m_answerCount) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }
    if (m_networkPort && !m_networkPort->IsConnected()) {
        return std::unexpected(Client::Core::CommandError::Disconnected);
    }

    if (m_networkPort) {
        TPacketCGScriptAnswer packet{};
        packet.header = CG::SCRIPT_ANSWER;
        packet.length = static_cast<uint16_t>(sizeof(packet));
        packet.answer = cmd.answerIndex;
        
        std::span<const uint8_t> payload(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        auto result = m_networkPort->SendRaw(packet.header, payload);
        if (!result.has_value()) {
            EterBase::ModernLogger::Error("QuestCommandHandler: Failed to send QuestAnswerCommand");
        }
    }

    ClearActiveScriptDialog();
    return {};
}

Client::Core::Result<void, Client::Core::CommandError> QuestCommandHandler::Execute(const Client::Core::QuestConfirmCommand& cmd) {
    if (!m_activeConfirmDialog) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }
    if (cmd.requestPID != m_requestPID) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }
    if (m_networkPort && !m_networkPort->IsConnected()) {
        return std::unexpected(Client::Core::CommandError::Disconnected);
    }

    if (m_networkPort) {
        TPacketCGQuestConfirm packet{};
        packet.header = CG::QUEST_CONFIRM;
        packet.length = static_cast<uint16_t>(sizeof(packet));
        packet.answer = cmd.answer;
        packet.requestPID = cmd.requestPID;
        
        std::span<const uint8_t> payload(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        auto result = m_networkPort->SendRaw(packet.header, payload);
        if (!result.has_value()) {
            EterBase::ModernLogger::Error("QuestCommandHandler: Failed to send QuestConfirmCommand");
        }
    }

    ClearActiveConfirmDialog();
    return {};
}

} // namespace Client::Gameplay
