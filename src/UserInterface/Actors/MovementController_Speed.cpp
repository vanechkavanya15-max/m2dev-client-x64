#include "../StdAfx.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"

#include <span>
#include <expected>
#include <memory>
#include <format>
#include <cstring>

namespace UserInterface::Actors::Movement {

/**
 * @brief Event published when the movement speed of an entity needs to be synchronized.
 * 
 * Local event definition to satisfy the Zero-Conflict Rule (not modifying existing headers).
 */
struct MovementSpeedSyncEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId entityId;
    uint16_t movementSpeed;

    /**
     * @brief Constructs the MovementSpeedSyncEvent.
     * @param id The entity ID whose speed is changing.
     * @param speed The new movement speed.
     */
    MovementSpeedSyncEvent(EterBase::EntityId id, uint16_t speed) 
        : entityId(id), movementSpeed(speed) {}
};

/**
 * @brief Class responsible for handling movement speed synchronization.
 * 
 * Adheres to the Single Responsibility Principle and modern C++23 standards.
 */
class MovementController_Speed {
public:
    /**
     * @brief Handles the synchronization of animation speed with movement speed.
     * 
     * Parses the incoming packet payload and publishes an event to the EventBus
     * to decouple the logic from the GUI and other systems.
     * 
     * @param payload The binary payload of the network packet.
     * @return EterBase::PacketResult<void> Returns success or a PacketError on failure.
     */
    static EterBase::PacketResult<void> HandleMovementSpeedSync(std::span<const uint8_t> payload) {
        if (payload.size() < sizeof(TPacketGCChangeSpeed)) {
            EterBase::ModernLogger::Error("MovementController_Speed: Payload size {} is smaller than required {}", 
                                          payload.size(), sizeof(TPacketGCChangeSpeed));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCChangeSpeed packet;
        std::memcpy(&packet, payload.data(), sizeof(TPacketGCChangeSpeed));
        
        EterBase::EntityId vid{packet.vid};
        
        EterBase::ModernLogger::Info("MovementController_Speed: Syncing animation speed for VID: {}, speed: {}", 
                                     packet.vid, packet.moving_speed);

        UserInterface::Core::EventBus::GetInstance().Publish(MovementSpeedSyncEvent{vid, packet.moving_speed});

        return {};
    }
};

} // namespace UserInterface::Actors::Movement
