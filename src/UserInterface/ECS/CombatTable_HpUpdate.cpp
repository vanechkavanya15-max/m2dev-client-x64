#include "../StdAfx.h"
#include "CombatComponentTable.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"

#include <algorithm>
#include <ranges>

namespace UserInterface::ECS
{
    /**
     * @brief Aktualizuje punkty zycia encji w tabeli komponentow walki i rozsyla powiadomienie do GUI.
     * 
     * @param table Referencja do tabeli komponentow walki.
     * @param entityId Identyfikator encji, ktorej dotyczy aktualizacja.
     * @param currentHp Nowa wartosc obecnego HP.
     * @param maxHp Nowa wartosc maksymalnego HP.
     * @return EterBase::Result<void, EterBase::EntityError> Wynik operacji lub blad domenowy.
     */
    EterBase::Result<void, EterBase::EntityError> UpdateEntityHp(
        CombatComponentTable& table,
        EterBase::EntityId entityId,
        uint32_t currentHp,
        uint32_t maxHp)
    {
        if (!entityId)
        {
            EterBase::ModernLogger::Error("UpdateEntityHp: Invalid entity ID.");
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        auto it = std::ranges::find(table.entityIds, entityId.value());
        if (it == table.entityIds.end())
        {
            EterBase::ModernLogger::Warning("UpdateEntityHp: Entity {} not found in CombatComponentTable.", entityId.value());
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        size_t idx = std::distance(table.entityIds.begin(), it);

        table.currentHp[idx] = currentHp;
        table.maxHp[idx] = maxHp;

        // Obliczenie procentowej zawartosci paska zycia (0-100)
        uint8_t hpPercentage = 0;
        if (maxHp > 0)
        {
            hpPercentage = static_cast<uint8_t>(std::clamp((currentHp * 100ull) / maxHp, 0ull, 100ull));
        }

        EterBase::ModernLogger::Debug("UpdateEntityHp: Entity {} HP updated to {}/{} ({}%)", 
            entityId.value(), currentHp, maxHp, hpPercentage);

        // Powiadomienie GUI / EventBus
        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Core::TargetBoardRefreshEvent(entityId.value())
        );

        return {};
    }
}
