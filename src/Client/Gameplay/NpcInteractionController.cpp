#include "NpcInteractionController.h"
#include <array>

namespace Client::Gameplay {

NpcInteractionController::NpcInteractionController(std::shared_ptr<Client::Core::INetworkPort> networkPort) noexcept
    : m_networkPort(std::move(networkPort)) {
}

Client::Core::Result<void, Client::Core::CommandError> NpcInteractionController::InteractWithNpc(const Client::Core::InteractNpcCommand& cmd) noexcept {
    if (!m_networkPort) {
        return std::unexpected(Client::Core::CommandError::Disconnected);
    }
    if (!m_networkPort->IsConnected()) {
        return std::unexpected(Client::Core::CommandError::Disconnected);
    }

    if (cmd.npcVid.value() == 0) {
        return std::unexpected(Client::Core::CommandError::InvalidTarget);
    }

    // Struktura pakietu: VID (4 bajty) + Akcja (1 bajt)
    // Naglowek: HEADER_CG_ON_CLICK = 26 (przekazywany osobno do SendRaw)
    constexpr uint8_t HEADER_CG_ON_CLICK = 26;
    std::array<uint8_t, 5> payload{};
    
    // Konwersja VID na little-endian
    uint32_t vid = cmd.npcVid.value();
    payload[0] = static_cast<uint8_t>(vid & 0xFF);
    payload[1] = static_cast<uint8_t>((vid >> 8) & 0xFF);
    payload[2] = static_cast<uint8_t>((vid >> 16) & 0xFF);
    payload[3] = static_cast<uint8_t>((vid >> 24) & 0xFF);
    
    payload[4] = cmd.actionType;

    auto result = m_networkPort->SendRaw(HEADER_CG_ON_CLICK, payload);
    if (!result) {
        return std::unexpected(Client::Core::CommandError::Disconnected);
    }

    return {};
}

} // namespace Client::Gameplay
