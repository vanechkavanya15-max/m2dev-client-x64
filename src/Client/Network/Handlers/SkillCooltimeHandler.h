#pragma once

#include <cstdint>
#include <span>
#include <expected>

#include "../../../UserInterface/Core/EventBus.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/Result.h"

namespace Client::Gameplay {
    class SkillDomain;
}

#pragma pack(push, 1)
/**
 * @brief Represents the packet received from the server when a skill cooldown ends.
 * Keeps strict memory alignment matching the legacy network protocol.
 */
struct TPacketGCSkillCoolTimeEnd
{
    /** @brief Network packet header identifier. */
    uint16_t header;
    /** @brief The length of the packet. */
    uint16_t length;
    /** @brief The ID of the skill whose cooldown has ended. */
    uint8_t bSkill;
};
#pragma pack(pop)

/**
 * @brief Event emitted when a skill's cooltime ends.
 * Subsystems can subscribe to this event to update the UI or internal states.
 */
struct SkillCooltimeEndEvent : public UserInterface::Core::IEvent
{
    /** @brief The ID of the skill whose cooldown has ended. */
    EterBase::SkillId skillId{0};

    SkillCooltimeEndEvent() = default;
    explicit SkillCooltimeEndEvent(EterBase::SkillId id) : skillId(id) {}
};

/**
 * @brief Modern handler for processing skill cooltime network events.
 * Implements C++23 std::expected based error handling and StrongTypes.
 */
class SkillCooltimeHandler
{
public:
    /**
     * @brief Parses and handles the skill cooltime end packet.
     * 
     * @param buffer A view over the raw byte buffer received from the network.
     * @param skillDomain Reference to the player's skill domain manager.
     * @return EterBase::PacketResult<void> Returns success or an error code indicating the failure reason.
     */
    static EterBase::PacketResult<void> HandlePacket(std::span<const uint8_t> buffer, Client::Gameplay::SkillDomain& skillDomain);
};
