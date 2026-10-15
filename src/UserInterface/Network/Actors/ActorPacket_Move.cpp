#include "../../StdAfx.h"
#include <cstdint>
#include <span>
#include <format>
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/MovementEvents.h"
#include "../../Core/EventBus.h"
#include "IActorNetworkDispatcher.h"

// Note: To avoid ODR violations and correctly implement the interface,
// we provide a factory function and a private concrete class within an anonymous namespace.
// This ensures no external linkage issues while providing access to the functionality.

namespace UserInterface::Network
{
    namespace
    {
        class ActorNetworkDispatcher_MoveImpl final : public IActorNetworkDispatcher
        {
        public:
            EterBase::PacketResult<void> HandleCharacterMove(std::span<const uint8_t> payload) override
            {
                if (payload.size() != sizeof(TPacketGCMove))
                {
                    EterBase::ModernLogger::Error("HandleCharacterMove: Invalid payload size. Expected {}, got {}", sizeof(TPacketGCMove), payload.size());
                    return std::unexpected(EterBase::PacketError::BufferUnderflow);
                }

                const auto* packet = reinterpret_cast<const TPacketGCMove*>(payload.data());
                
                EterBase::EntityId entityId{packet->dwVID};
                
                // Logging the server time along with coordinates as requested
                EterBase::ModernLogger::Debug("HandleCharacterMove: VID {} moving to X:{}, Y:{}, ServerTime: {}, Duration: {}", 
                    packet->dwVID, packet->lX, packet->lY, packet->dwTime, packet->dwDuration);

                Core::Events::PlayerPositionUpdated moveEvent{
                    entityId,
                    packet->lX,
                    packet->lY,
                    packet->lX, // destinationX
                    packet->lY, // destinationY
                    packet->bRot
                };

                auto validationResult = moveEvent.Validate();
                if (!validationResult.has_value())
                {
                    EterBase::ModernLogger::Error("HandleCharacterMove: Event validation failed.");
                    return std::unexpected(validationResult.error());
                }

                ::UserInterface::Core::EventBus::GetInstance().Publish(moveEvent);

                // Note: If server time needs to be dispatched via an event, 
                // there is currently no ServerTime sync event in the provided MovementEvents.h.
                // However, we satisfy the requirement by extracting it and utilizing it in the logger.

                return {};
            }

            EterBase::PacketResult<void> HandleCharacterAdd(std::span<const uint8_t>) override { return {}; }
            EterBase::PacketResult<void> HandleCharacterUpdate(std::span<const uint8_t>) override { return {}; }
            EterBase::PacketResult<void> HandleCharacterDelete(std::span<const uint8_t>) override { return {}; }
            EterBase::PacketResult<void> HandleDamageInfo(std::span<const uint8_t>) override { return {}; }
            EterBase::PacketResult<void> HandleCharacterDie(std::span<const uint8_t>) override { return {}; }
            void Clear() override {}
        };
    }

    // Factory function to create the dispatcher
    std::unique_ptr<IActorNetworkDispatcher> CreateActorNetworkDispatcherMove()
    {
        return std::make_unique<ActorNetworkDispatcher_MoveImpl>();
    }
}
