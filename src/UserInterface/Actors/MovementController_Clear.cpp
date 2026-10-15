#include "../StdAfx.h"
#include <expected>
#include <cstdint>
#include <memory>

#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Packet.h"

namespace {

    /**
     * @brief Event emitted when an actor's movement buffers need to be cleared.
     * Inherits from IEvent. Not packed due to vtable from IEvent.
     */
    struct MovementClearedEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId entityId;

        /**
         * @brief Constructs the MovementClearedEvent.
         * @param entityId The unique identifier of the entity whose movement was cleared.
         */
        explicit MovementClearedEvent(EterBase::EntityId entityId) : entityId(entityId) {}
    };

} // anonymous namespace

namespace UserInterface::Actors {

    class MovementController {
    public:
        /**
         * @brief Clears the movement buffers for a specific actor instance.
         * 
         * This ensures any pending movement operations or path queues are flushed,
         * for example when a character is abruptly stopped, staggered, or teleported.
         * 
         * @param entityId The EntityId of the actor instance to clear.
         * @return EterBase::VoidResult<EterBase::EntityError> Success if cleared, or an error.
         */
        [[nodiscard]] static std::expected<void, EterBase::EntityError> ClearMovementBuffers(EterBase::EntityId entityId) noexcept {
            if (entityId.value() == 0) {
                EterBase::ModernLogger::Error("MovementController::ClearMovementBuffers failed: invalid EntityId (0).");
                return std::unexpected(EterBase::EntityError::NotFound);
            }

            EterBase::ModernLogger::Info("MovementController::ClearMovementBuffers clearing movement queue for entity: {}.", entityId.value());

            // Emit the event to inform GUI / Other Subsystems without direct Python coupling
            UserInterface::Core::EventBus::GetInstance().Publish(MovementClearedEvent(entityId));

            return {};
        }
    };

} // namespace UserInterface::Actors
