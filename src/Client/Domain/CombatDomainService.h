#pragma once

#include <vector>
#include <optional>
#include <span>
#include <EterBase/StrongTypes.h>
#include <EterBase/Result.h>

namespace Client::Domain {

/**
 * @brief Struktura reprezentujaca rzadanie wykonania ataku w systemie walki.
 */
struct StrikeRequest {
    EterBase::EntityId attackerId;
    EterBase::EntityId targetId;
};

/**
 * @brief Wynik kalkulacji trafienia.
 */
struct HitCalculationResult {
    int32_t damage;
    bool isCritical;
    bool isPiercing;
};

/**
 * @brief Serwis domenowy odpowiedzialny za logike walki.
 * Zarzadza kolejkowaniem ciosow, celownikiem i podstawa kalkulacji obrazen.
 */
class CombatDomainService {
public:
    constexpr CombatDomainService() noexcept = default;

    // ========================================================================
    // Zarzadzanie Celownikiem (Targeting)
    // ========================================================================

    /**
     * @brief Ustawia aktualny cel dla gracza.
     * @param targetId Identyfikator celu (nie moze byc pusty).
     * @return Sukces lub blad CombatError::InvalidAction.
     */
    constexpr EterBase::Result<void, EterBase::CombatError> SetTarget(EterBase::EntityId targetId) noexcept {
        if (!targetId) {
            return EterBase::MakeError(EterBase::CombatError::InvalidAction);
        }
        m_currentTarget = targetId;
        return {};
    }

    /**
     * @brief Czysty stan celownika.
     */
    constexpr void ClearTarget() noexcept {
        m_currentTarget.reset();
    }

    /**
     * @brief Pobiera aktualny identyfikator celu.
     * @return std::optional z EntityId jesli cel jest ustawiony.
     */
    [[nodiscard]] constexpr std::optional<EterBase::EntityId> GetCurrentTarget() const noexcept {
        return m_currentTarget;
    }

    // ========================================================================
    // Kolejkowanie Ciosow (Strike Queuing)
    // ========================================================================

    /**
     * @brief Dodaje atak do kolejki przetwarzania.
     * @param attackerId Identyfikator atakujacego.
     * @param targetId Identyfikator ofiary.
     * @return Sukces lub blad jesli ktorys z ID jest niewazny lub propozycja samobojstwa.
     */
    constexpr EterBase::Result<void, EterBase::CombatError> EnqueueStrike(EterBase::EntityId attackerId, EterBase::EntityId targetId) {
        if (!attackerId || !targetId) {
            return EterBase::MakeError(EterBase::CombatError::InvalidAction);
        }
        if (attackerId == targetId) {
            // Brak mozliwosci ataku samego siebie.
            return EterBase::MakeError(EterBase::CombatError::InvalidAction);
        }

        m_strikeQueue.push_back(StrikeRequest{attackerId, targetId});
        return {};
    }

    /**
     * @brief Pobiera widok na aktualna kolejke ciosow bez jej modyfikacji.
     * @return Widok (std::span) na zadania ataku.
     */
    [[nodiscard]] constexpr std::span<const StrikeRequest> GetQueuedStrikes() const noexcept {
        return m_strikeQueue;
    }

    /**
     * @brief Czysci calkowicie kolejke ciosow.
     */
    constexpr void ClearQueuedStrikes() noexcept {
        m_strikeQueue.clear();
    }

    // ========================================================================
    // Kalkulacja Trafien (Hit Calculation)
    // ========================================================================

    /**
     * @brief Wylicza parametry trafienia dla danego rzadania.
     * W tej wersji implementacja bazowa zakladajaca stale obrazenia testowe.
     * @param request Instancja rzadania ataku.
     * @return Sukces zawierajacy HitCalculationResult lub kod bledu CombatError.
     */
    [[nodiscard]] constexpr EterBase::Result<HitCalculationResult, EterBase::CombatError> CalculateHit(const StrikeRequest& request) const noexcept {
        if (!request.attackerId || !request.targetId) {
            return EterBase::MakeError(EterBase::CombatError::InvalidAction);
        }
        if (request.attackerId == request.targetId) {
            return EterBase::MakeError(EterBase::CombatError::InvalidAction);
        }

        // Prosta kalkulacja placeholderowa dla zachowania wymogow Headless i C++23.
        // Oparta na identyfikatorach aby zapewnic determinizm testow.
        HitCalculationResult result{};
        result.damage = 100 + (request.attackerId.value() % 10);
        result.isCritical = (request.attackerId.value() % 5 == 0);
        result.isPiercing = (request.attackerId.value() % 7 == 0);

        return result;
    }

private:
    std::optional<EterBase::EntityId> m_currentTarget;
    std::vector<StrikeRequest> m_strikeQueue;
};

} // namespace Client::Domain
