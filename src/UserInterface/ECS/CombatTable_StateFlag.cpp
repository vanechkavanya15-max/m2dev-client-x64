#include "../StdAfx.h"
#include "CombatComponentTable.h"
#include "../Packet.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <algorithm>
#include <expected>

namespace UserInterface::ECS::CombatStateFlag
{
    /**
     * @brief Flagi reprezentujace stan walki encji.
     */
    enum class Flag : uint8_t
    {
        None     = 0,
        InCombat = 1 << 0,
        Stunned  = 1 << 1,
        Immune   = 1 << 2
    };

    /**
     * @brief Dodaje flage do stanu walki danej encji.
     * @param table Referencja do tabeli komponentow walki.
     * @param entityId Silny typ identyfikatora encji.
     * @param flag Flaga do dodania.
     * @return Oczekiwany rezultat bezwartosciowy (std::expected), lub blad EntityError.
     */
    std::expected<void, EterBase::EntityError> AddFlag(CombatComponentTable& table, EterBase::EntityId entityId, Flag flag)
    {
        auto it = std::find(table.entityIds.begin(), table.entityIds.end(), entityId.get());
        if (it == table.entityIds.end())
        {
            EterBase::ModernLogger::Warn("CombatStateFlag::AddFlag - Entity {} not found.", entityId.get());
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        size_t idx = std::distance(table.entityIds.begin(), it);
        if (table.isDead[idx])
        {
            EterBase::ModernLogger::Warn("CombatStateFlag::AddFlag - Entity {} is dead.", entityId.get());
            return std::unexpected(EterBase::EntityError::Dead);
        }

        uint8_t oldState = table.battleState[idx];
        table.battleState[idx] |= static_cast<uint8_t>(flag);

        if (oldState != table.battleState[idx])
        {
            EterBase::ModernLogger::Info("Entity {} combat state changed. Added flag. Old: {}, New: {}", 
                                         entityId.get(), oldState, table.battleState[idx]);
            
            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(entityId.get()));
        }

        return {};
    }

    /**
     * @brief Usuwa flage ze stanu walki danej encji.
     * @param table Referencja do tabeli komponentow walki.
     * @param entityId Silny typ identyfikatora encji.
     * @param flag Flaga do usuniecia.
     * @return Oczekiwany rezultat bezwartosciowy (std::expected), lub blad EntityError.
     */
    std::expected<void, EterBase::EntityError> RemoveFlag(CombatComponentTable& table, EterBase::EntityId entityId, Flag flag)
    {
        auto it = std::find(table.entityIds.begin(), table.entityIds.end(), entityId.get());
        if (it == table.entityIds.end())
        {
            EterBase::ModernLogger::Warn("CombatStateFlag::RemoveFlag - Entity {} not found.", entityId.get());
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        size_t idx = std::distance(table.entityIds.begin(), it);
        
        uint8_t oldState = table.battleState[idx];
        table.battleState[idx] &= ~static_cast<uint8_t>(flag);

        if (oldState != table.battleState[idx])
        {
            EterBase::ModernLogger::Info("Entity {} combat state changed. Removed flag. Old: {}, New: {}", 
                                         entityId.get(), oldState, table.battleState[idx]);
            
            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(entityId.get()));
        }

        return {};
    }

    /**
     * @brief Sprawdza czy encja posiada dana flage.
     * @param table Referencja do tabeli komponentow walki.
     * @param entityId Silny typ identyfikatora encji.
     * @param flag Flaga do sprawdzenia.
     * @return Oczekiwana wartosc logiczna (true/false) lub blad EntityError.
     */
    std::expected<bool, EterBase::EntityError> HasFlag(const CombatComponentTable& table, EterBase::EntityId entityId, Flag flag)
    {
        auto it = std::find(table.entityIds.begin(), table.entityIds.end(), entityId.get());
        if (it == table.entityIds.end())
        {
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        size_t idx = std::distance(table.entityIds.begin(), it);
        return (table.battleState[idx] & static_cast<uint8_t>(flag)) != 0;
    }

    /**
     * @brief Przelacza stan flagi (dodaje jesli brak, usuwa jesli jest) danej encji.
     * @param table Referencja do tabeli komponentow walki.
     * @param entityId Silny typ identyfikatora encji.
     * @param flag Flaga do przelaczenia.
     * @return Oczekiwany rezultat bezwartosciowy (std::expected), lub blad EntityError.
     */
    std::expected<void, EterBase::EntityError> ToggleFlag(CombatComponentTable& table, EterBase::EntityId entityId, Flag flag)
    {
        auto hasFlagResult = HasFlag(table, entityId, flag);
        if (!hasFlagResult.has_value())
        {
            return std::unexpected(hasFlagResult.error());
        }

        if (hasFlagResult.value())
        {
            return RemoveFlag(table, entityId, flag);
        }
        else
        {
            return AddFlag(table, entityId, flag);
        }
    }
}
