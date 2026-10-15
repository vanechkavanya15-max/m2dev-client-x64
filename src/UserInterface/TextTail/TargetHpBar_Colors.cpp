#include "../StdAfx.h"
#include "ITargetHpBarService.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Domain/ActorRegistryModel.h"
#include "../../GameLib/RaceManagerLite.h"

#include <cstdint>
#include <optional>
#include <expected>

namespace UserInterface::TextTail
{
    /**
     * @brief Service for determining the color of a target's HP bar based on actor type and race.
     * 
     * Applies SRP by decoupling color logic from the UI rendering and targeting components,
     * and integrates with the EventBus to trigger UI refreshes when evaluated.
     */
    class TargetHpBarColorService
    {
    public:
        TargetHpBarColorService() = delete;
        ~TargetHpBarColorService() = delete;

        /**
         * @brief Determines the ARGB color code for the target HP bar and notifies the UI.
         * 
         * @param vid The unique entity ID of the target.
         * @param actorType The fundamental type of the actor (e.g. Player, Enemy, Stone).
         * @param raceVnum Optional race identifier (VNUM) used to check for Boss status.
         * @return std::expected<uint32_t, EterBase::EntityError> Expected containing ARGB color code or an error.
         */
        [[nodiscard]] static std::expected<uint32_t, EterBase::EntityError> DetermineColor(
            EterBase::EntityId vid,
            Domain::ActorType actorType,
            std::optional<uint32_t> raceVnum = std::nullopt)
        {
            uint32_t color = 0xFFFFFFFF; // Domyslny (Bialy)

            if (actorType == Domain::ActorType::Player)
            {
                color = 0xFF00FF00; // Zielony
            }
            else if (raceVnum.has_value() && RaceManagerLite::GetInstance().IsBoss(raceVnum.value()))
            {
                color = 0xFF800080; // Fioletowy
            }
            else if (actorType == Domain::ActorType::Enemy || actorType == Domain::ActorType::Stone)
            {
                color = 0xFFFF0000; // Czerwony
            }

            EterBase::ModernLogger::Info("Determined HP bar color for entity {} as {}", vid.value(), color);
            
            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(vid.value()));
            
            return color;
        }
    };
} // namespace UserInterface::TextTail
