#include <string_view>
#pragma once

#include <expected>
#include <optional>
#include <array>
#include <cstdint>
#include <format>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

/**
 * @file StatPointAllocatorOptimizer.h
 * @brief Implementacja optymalnego algorytmu rozdawania punktów statusu dla buildów.
 * 
 * Ten moduł odpowiada za dystrybucję dostępnych punktów statusu na podstawie pożądanej
 * alokacji, zachowując restrykcje (np. maksymalny poziom poszczególnych statystyk)
 * oraz obsługując błędy domenowe. Zgodnie z zasadą Zero-Conflict, to samodzielny
 * komponent nie ingerujący w istniejący kod.
 */

namespace GameLib {

/**
 * @brief Identyfikator konkretnej statystyki (np. HP, SP, STR, DEX).
 * Wartości te są dostosowane do specyfikacji gry Metin2.
 */
enum class StatType : uint8_t {
    VIT = 0, ///< Vitality (HP, Def)
    INT = 1, ///< Intelligence (SP, M.Atk)
    STR = 2, ///< Strength (Atk)
    DEX = 3  ///< Dexterity (Evasion, Dmg)
};

/**
 * @brief Zdarzenie emitowane po pomyślnym przydzieleniu punktu statusu.
 * Pozwala na asynchroniczne odświeżenie interfejsu (np. Okno Postaci).
 */
struct StatPointAllocatedEvent : public UserInterface::Core::IEvent {
    EterBase::EntityId playerId;
    StatType type;
    uint8_t newAmount;
    uint16_t remainingPoints;

    /**
     * @brief Konstruktor zdarzenia alokacji statusu.
     * @param playerId ID gracza, którego dotyczy aktualizacja.
     * @param type Typ statystyki, która została podniesiona.
     * @param newAmount Nowa wartość danej statystyki po alokacji.
     * @param remainingPoints Pozostała liczba wolnych punktów statusu.
     */
    StatPointAllocatedEvent(EterBase::EntityId playerId, StatType type, uint8_t newAmount, uint16_t remainingPoints)
        : playerId(playerId), type(type), newAmount(newAmount), remainingPoints(remainingPoints) {}
};

/**
 * @brief Błędy mogące wystąpić podczas alokacji punktów statusu.
 */
enum class StatAllocationError : uint8_t {
    None = 0,
    InsufficientPoints, ///< Gracz nie posiada wolnych punktów statusu
    StatMaxedOut,       ///< Zmiana przekracza maksymalną dopuszczalną wartość statystyki
    InvalidStatType     ///< Podano nieznany lub nieobsługiwany typ statystyki
};

[[nodiscard]] constexpr std::string_view ToString(StatAllocationError err) noexcept {
    switch (err) {
        case StatAllocationError::None: return "None";
        case StatAllocationError::InsufficientPoints: return "InsufficientPoints";
        case StatAllocationError::StatMaxedOut: return "StatMaxedOut";
        case StatAllocationError::InvalidStatType: return "InvalidStatType";
    }
    return "UnknownStatAllocationError";
}

/**
 * @brief Pomocniczy alias resultatu dla alokacji statusu.
 */
using StatAllocationResult = std::expected<void, StatAllocationError>;

} // namespace GameLib

template <>
struct std::formatter<GameLib::StatAllocationError> : std::formatter<std::string_view> {
    auto format(GameLib::StatAllocationError err, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(GameLib::ToString(err), ctx);
    }
};

namespace GameLib {

/**
 * @brief Optymalizator dystrybucji punktów statusu.
 * Posiada algorytmy pozwalające optymalnie rozdzielić wolne punkty 
 * na podstawie zadanego profilu lub priorytetów (np. 2 VIT : 1 INT).
 */
class StatPointAllocatorOptimizer {
public:
    static constexpr uint8_t MAX_STAT_VALUE = 90;

    /**
     * @brief Przydziela jeden punkt statusu do wybranej statystyki.
     * 
     * @param playerId Silnie typowane ID gracza ubiegającego się o alokację.
     * @param type Wybrany typ statystyki.
     * @param currentStatValue Obecna wartość danej statystyki na graczu.
     * @param availablePoints Liczba wolnych punktów (np. z poziomu postaci).
     * 
     * @return StatAllocationResult Zwraca pusty std::expected jeśli się powiodło, lub StatAllocationError.
     */
    [[nodiscard]] static StatAllocationResult AllocateSinglePoint(
        EterBase::EntityId playerId, 
        StatType type, 
        uint8_t currentStatValue, 
        uint16_t availablePoints) 
    {
        if (availablePoints == 0) {
            EterBase::ModernLogger::Warn("StatPointAllocatorOptimizer::AllocateSinglePoint - EntityId {} has no available stat points.", playerId.get());
            return std::unexpected(StatAllocationError::InsufficientPoints);
        }

        if (currentStatValue >= MAX_STAT_VALUE) {
            EterBase::ModernLogger::Warn("StatPointAllocatorOptimizer::AllocateSinglePoint - EntityId {} tried to increase maxed stat ({}).", playerId.get(), static_cast<uint8_t>(type));
            return std::unexpected(StatAllocationError::StatMaxedOut);
        }

        uint8_t newAmount = currentStatValue + 1;
        uint16_t remainingPoints = availablePoints - 1;

        // Emitowanie zdarzenia na EventBus - powiadamiamy inne systemy (w tym GUI)
        UserInterface::Core::EventBus::GetInstance().Publish(
            StatPointAllocatedEvent(playerId, type, newAmount, remainingPoints)
        );

        EterBase::ModernLogger::Info("StatPointAllocatorOptimizer::AllocateSinglePoint - EntityId {} allocated 1 point to stat type {}. New value: {}, Remaining points: {}", 
            playerId.get(), static_cast<uint8_t>(type), newAmount, remainingPoints);

        return {};
    }

    /**
     * @brief Definiuje priorytet przyznawania statystyk dla auto-przydziału.
     * Przydatne dla buildów typu "Mental" (np. VIT > STR > DEX).
     */
    using StatPriorityList = std::array<StatType, 4>;

    /**
     * @brief Przydziela całą pule dostępnych punktów na podstawie priorytetów.
     * Przydziela 1 punkt do statystyki o najwyższym priorytecie. Jeśli
     * osiągnie ona maksimum, przechodzi do kolejnej na liście.
     *
     * @param playerId ID gracza.
     * @param currentStats Tablica reprezentująca obecny stan statystyk (VIT, INT, STR, DEX).
     * @param availablePoints Liczba wolnych punktów statusu.
     * @param priorities Tablica z priorytetami od najwyższego (indeks 0) do najniższego.
     * 
     * @return StatAllocationResult Zwraca sukces (częściowy lub pełny przydział), lub błąd jeśli brak punktów.
     */
    [[nodiscard]] static StatAllocationResult AutoAllocatePoints(
        EterBase::EntityId playerId,
        std::array<uint8_t, 4>& currentStats, 
        uint16_t& availablePoints,
        const StatPriorityList& priorities)
    {
        if (availablePoints == 0) {
            return std::unexpected(StatAllocationError::InsufficientPoints);
        }

        uint16_t totalAllocated = 0;

        while (availablePoints > 0) {
            bool pointAllocatedInCycle = false;

            for (const auto& statType : priorities) {
                size_t statIdx = static_cast<size_t>(statType);

                // Skip unsupported just in case
                if (statIdx >= 4) continue;

                if (currentStats[statIdx] < MAX_STAT_VALUE) {
                    currentStats[statIdx]++;
                    availablePoints--;
                    totalAllocated++;
                    pointAllocatedInCycle = true;

                    UserInterface::Core::EventBus::GetInstance().Publish(
                        StatPointAllocatedEvent(playerId, statType, currentStats[statIdx], availablePoints)
                    );

                    break; 
                }
            }

            // Jeśli wszystkie z listy priorytetów są wymaksowane
            if (!pointAllocatedInCycle) {
                break;
            }
        }

        EterBase::ModernLogger::Info("StatPointAllocatorOptimizer::AutoAllocatePoints - EntityId {} auto-allocated {} points.", playerId.get(), totalAllocated);

        return {};
    }
};

} // namespace GameLib
