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
     * @brief Handler for GC party packets (HEADER_GC_PARTY_INVITE, ADD, UPDATE, REMOVE).
     * Synchronizes party state directly into Gameplay::SocialManager and adheres to Zero-Conflict principles.
     */
    class PartyHandler {
    public:
        explicit PartyHandler(std::shared_ptr<Client::Gameplay::SocialManager> socialManager);
        ~PartyHandler() = default;

        EterBase::PacketResult<void> HandlePartyInvite(std::span<const uint8_t> payload);
        EterBase::PacketResult<void> HandlePartyAdd(std::span<const uint8_t> payload);
        EterBase::PacketResult<void> HandlePartyUpdate(std::span<const uint8_t> payload);
        EterBase::PacketResult<void> HandlePartyRemove(std::span<const uint8_t> payload);

    private:
        std::shared_ptr<Client::Gameplay::SocialManager> m_socialManager;
    };

} // namespace Client::Network::Handlers
