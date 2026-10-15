#pragma once

#include <cstdint>
#include <span>
#include <string>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Gameplay/SocialDomain.h"

namespace Client::Network::Handlers {

/**
 * @brief Handles guild and mark update network packets.
 * Deserializes GUILD and MARK_UPDATE packets and updates SocialManager.
 */
class GuildHandler {
public:
    explicit GuildHandler(Client::Gameplay::SocialManager& socialManager);

    /**
     * @brief Parses GC::GUILD packet and subpackets.
     */
    EterBase::PacketResult<void> HandleGuildPacket(std::span<const uint8_t> payload);

    /**
     * @brief Parses GC::MARK_UPDATE packet.
     */
    EterBase::PacketResult<void> HandleMarkUpdatePacket(std::span<const uint8_t> payload);

private:
    Client::Gameplay::SocialManager& m_socialManager;
};

// Events for EventBus decoupling
struct GuildBankUpdatedEvent {
    uint32_t gold;
    explicit GuildBankUpdatedEvent(uint32_t gold) : gold(gold) {}
};

struct GuildExpUpdatedEvent {
    uint8_t level;
    uint32_t exp;
    GuildExpUpdatedEvent(uint8_t level, uint32_t exp) : level(level), exp(exp) {}
};

struct GuildMarkUpdatedEvent {
    uint32_t guildId;
    uint16_t markIndex;
    GuildMarkUpdatedEvent(uint32_t guildId, uint16_t markIndex) : guildId(guildId), markIndex(markIndex) {}
};

} // namespace Client::Network::Handlers
