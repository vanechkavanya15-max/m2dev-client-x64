#pragma once

#include <cstdint>
#include <memory>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "UserInterface/PlayerControllers/PlayerTargetController.h"
#include "UserInterface/TextTail/ITargetHpBarService.h"
#include "Client/Gameplay/ExchangeCommandHandler.h"

namespace Client::Actor {

/**
 * @brief Enum definiujacy domeny czyszczenia referencji dla znikajacego aktora.
 */
enum class CleanDomain : uint8_t {
    TargetSelection = 1 << 0,
    TargetHpBar     = 1 << 1,
    ExchangeWindow  = 1 << 2,
    All             = 0xFF
};

constexpr CleanDomain operator|(CleanDomain a, CleanDomain b) noexcept {
    return static_cast<CleanDomain>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr bool operator&(CleanDomain a, CleanDomain b) noexcept {
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

enum class CleanerError : uint8_t {
    InvalidEntityId,
    NullDependency
};

/**
 * @brief Komponent odpowiedzialny za czyszczenie zaznaczen, okien handlu i paskow HP
 *        po zniknieciu aktora (despawn). Implementacja C++23, zgodna z AI-First Architecture.
 */
class ActorDespawnCleaner {
public:
    ActorDespawnCleaner(
        UserInterface::PlayerControllers::PlayerTargetController* targetController = nullptr,
        UserInterface::TextTail::ITargetHpBarService* targetHpBarService = nullptr,
        Client::Gameplay::ExchangeCommandHandler* exchangeHandler = nullptr
    ) noexcept;
    
    ~ActorDespawnCleaner() = default;

    // Zero-Cost, non-copyable/non-movable
    ActorDespawnCleaner(const ActorDespawnCleaner&) = delete;
    ActorDespawnCleaner& operator=(const ActorDespawnCleaner&) = delete;

    void SetTargetController(UserInterface::PlayerControllers::PlayerTargetController* controller) noexcept { m_targetController = controller; }
    void SetTargetHpBarService(UserInterface::TextTail::ITargetHpBarService* service) noexcept { m_targetHpBarService = service; }
    void SetExchangeHandler(Client::Gameplay::ExchangeCommandHandler* handler) noexcept { m_exchangeHandler = handler; }

    /**
     * @brief Glowna metoda czyszczaca referencje po despawnie.
     * @param deadActorId Identyfikator znikajacego aktora.
     * @param domains Domeny, ktore nalezy wyczyscic (domyslnie wszystkie).
     * @return EterBase::Result z ewentualnym bledem.
     */
    EterBase::Result<void, CleanerError> CleanReferences(
        EterBase::EntityId deadActorId, 
        CleanDomain domains = CleanDomain::All) const noexcept;

private:
    void CleanTargetSelection(EterBase::EntityId deadActorId) const noexcept;
    void CleanTargetHpBar(EterBase::EntityId deadActorId) const noexcept;
    void CleanExchangeWindow(EterBase::EntityId deadActorId) const noexcept;

    UserInterface::PlayerControllers::PlayerTargetController* m_targetController{nullptr};
    UserInterface::TextTail::ITargetHpBarService* m_targetHpBarService{nullptr};
    Client::Gameplay::ExchangeCommandHandler* m_exchangeHandler{nullptr};
};

} // namespace Client::Actor
