#include "QuestDialogPacketHandler.h"
#include "../QuestPacketCodec.h"
#include <cstring> // Dla strnlen

namespace Client::Network::Handlers
{
    EterBase::PacketResult<void> QuestDialogPacketHandler::HandleQuestConfirm(std::span<const uint8_t> buffer) const noexcept
    {
        auto result = QuestPacketCodec::DecodeQuestConfirm(buffer);
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }

        const auto& packet = result.value();
        
        // Kopiowanie bezpieczne znakow z zachowaniem null terminatora
        std::string message(packet.msg, strnlen(packet.msg, sizeof(packet.msg)));

        Client::Core::EventBus::GetInstance().Publish(QuestConfirmEvent{
            std::move(message),
            packet.timeout,
            packet.requestPID
        });

        return {};
    }

    EterBase::PacketResult<void> QuestDialogPacketHandler::HandleQuestScript(std::span<const uint8_t> buffer) const noexcept
    {
        auto result = QuestPacketCodec::DecodeScript(buffer);
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }

        Client::Core::EventBus::GetInstance().Publish(QuestScriptEvent{
            result.value()
        });

        return {};
    }

} // namespace Client::Network::Handlers
