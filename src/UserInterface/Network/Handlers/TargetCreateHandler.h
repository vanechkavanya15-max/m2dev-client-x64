#pragma once

#include <cstdint>
#include <span>
#include <string>
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

namespace Network::Handlers
{
#pragma pack(push, 1)
    /**
     * @brief Structure representing the target creation packet payload from the server.
     */
    struct PacketTargetCreateNew
    {
        uint16_t header;            ///< Packet identifier
        uint16_t length;            ///< Packet length
        int32_t  id;                ///< Target identifier
        char     targetName[32+1];  ///< Name of the target
        uint32_t vid;               ///< Virtual ID of the target
        uint8_t  type;              ///< Type of the target
    };
#pragma pack(pop)

    /**
     * @brief Target creation types.
     */
    enum class TargetCreateType : uint8_t
    {
        None = 0,
        Location = 1,
        Character = 2
    };

    /**
     * @brief Event emitted when a new target is created.
     * 
     * Replaces direct Python UI calls to achieve decoupling.
     */
    struct TargetCreateEvent : public UserInterface::Core::IEvent {
        int32_t id;
        std::string targetName;
        EterBase::EntityId vid;
        TargetCreateType type;

        TargetCreateEvent(int32_t id, std::string name, EterBase::EntityId vid, TargetCreateType type)
            : id(id), targetName(std::move(name)), vid(vid), type(type) {}
    };

    /**
     * @brief Processes the target create packet and updates the C++ memory state.
     * 
     * @param buffer Binary view representing the incoming network packet.
     * @return EterBase::PacketResult<void> with a success or error status.
     */
    EterBase::PacketResult<void> ProcessTargetCreate(std::span<const uint8_t> buffer);

    /**
     * @brief Compatible interface for processing the target create packet.
     * 
     * @param buffer Binary view representing the incoming network packet.
     * @return true if the packet was successfully parsed and applied; otherwise false.
     */
    bool HandleTargetCreate(std::span<const uint8_t> buffer);
}
