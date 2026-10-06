#include "../StdAfx.h"
#include "ITextTailService.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "Core/EventBus.h"

namespace UserInterface::TextTail
{
    /**
     * @brief Event emitted when a text tail is removed.
     * Defined locally to adhere to the Zero-Conflict Rule.
     */
    struct TextTailRemovedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId;

        explicit TextTailRemovedEvent(EterBase::EntityId id) : entityId(id) {}
    };

    /**
     * @brief Handler for safely removing text tails from the scene.
     * Adheres to SRP and Zero-Conflict architectural guidelines.
     */
    class TextTailRemoveHandler
    {
    public:
        /**
         * @brief Removes a text tail for a given entity and broadcasts the event.
         * 
         * @param service The text tail service interface.
         * @param entityId The unique identifier of the entity.
         * @return std::expected<void, EterBase::EntityError> Success or error.
         */
        static std::expected<void, EterBase::EntityError> Execute(ITextTailService& service, EterBase::EntityId entityId)
        {
            if (!entityId)
            {
                EterBase::ModernLogger::Warn("TextTailRemoveHandler: Attempted to remove tail for invalid EntityId.");
                return std::unexpected(EterBase::EntityError::NotFound);
            }

            // Remove from service
            service.RemoveTail(entityId.value());

            // Emit event without heap allocation
            TextTailRemovedEvent event(entityId);
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            // Log success
            EterBase::ModernLogger::Debug("TextTailRemoveHandler: Successfully removed text tail for VID: {}", entityId.value());

            return {};
        }
    };
}
