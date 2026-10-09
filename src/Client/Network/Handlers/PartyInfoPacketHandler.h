#pragma once

#include <memory>
#include <span>
#include <cstdint>
#include "../../../EterBase/PacketResult.h"

namespace Client::Gameplay {
    class SocialManager;
}

namespace Client::Network::Handlers {

    /**
     * @brief Handler for GC party info packets (HEADER_GC_PARTY_LINK, UNLINK).
     * Synchronizes party member positions and states on the minimap by linking PIDs to VIDs.
     * Adheres to Zero-Conflict principles and modern C++23 standards.
     */
    class PartyInfoPacketHandler {
    public:
        explicit PartyInfoPacketHandler(std::shared_ptr<Client::Gameplay::SocialManager> socialManager);
        ~PartyInfoPacketHandler() = default;

        EterBase::PacketResult<void> HandlePartyLink(std::span<const uint8_t> payload);
        EterBase::PacketResult<void> HandlePartyUnlink(std::span<const uint8_t> payload);

    private:
        std::shared_ptr<Client::Gameplay::SocialManager> m_socialManager;
    };

} // namespace Client::Network::Handlers
