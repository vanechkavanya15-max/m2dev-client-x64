#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include "Client/Gameplay/TradeDomain.h"
#include "EterBase/Result.h"

namespace Client::Domain {

/**
 * @class TradeDomainService
 * @brief Fasada serwisowa obslugujaca proces handlu (wymiana miedzy graczami oraz sklep NPC).
 * 
 * Komponent ukrywa zlozona logike domenowa `PlayerExchange` oraz `NpcShop`.
 * Zaprojektowany z mysla o architekturze asynchronicznej (C++23).
 * Wszystkie metody uzywaja `EterBase::Result` lub `EterBase::VoidResult`
 * w celu zapewnienia pelnego bezpieczenstwa wywolan (Memory Safety).
 */
class TradeDomainService {
public:
    TradeDomainService() = default;
    ~TradeDomainService() = default;

    // Usuwamy konstruktory kopiujace/przenoszace z racji zarzadzania zasobami unique_ptr
    TradeDomainService(const TradeDomainService&) = delete;
    TradeDomainService& operator=(const TradeDomainService&) = delete;
    TradeDomainService(TradeDomainService&&) noexcept = default;
    TradeDomainService& operator=(TradeDomainService&&) noexcept = default;

    // ========================================================================
    // API Wymiany Miedzy Graczami (Player Exchange)
    // ========================================================================

    /**
     * @brief Inicjuje nowa sesje wymiany pomiedzy dwoma graczami.
     */
    EterBase::VoidResult<std::string_view> InitiateExchange(EterBase::EntityId initiator, EterBase::EntityId target) noexcept {
        if (m_active_exchange) {
            return EterBase::MakeError("Inna wymiana jest juz aktywna");
        }
        if (initiator == target) {
            return EterBase::MakeError("Nie mozna handlowac z samym soba");
        }
        
        m_active_exchange = std::make_unique<Client::Gameplay::PlayerExchange>(initiator, target);
        return {};
    }

    /**
     * @brief Anuluje trwajaca wymiane (jezeli istnieje).
     */
    EterBase::VoidResult<std::string_view> CancelExchange() noexcept {
        if (!m_active_exchange) {
            return EterBase::MakeError("Brak aktywnej wymiany do anulowania");
        }
        
        auto res = m_active_exchange->Cancel();
        if (res.has_value()) {
            m_active_exchange.reset();
        }
        return res;
    }

    /**
     * @brief Dodaje przedmiot do wymiany dla danego gracza.
     */
    EterBase::VoidResult<std::string_view> AddExchangeItem(EterBase::EntityId player, EterBase::ItemSlot slot, EterBase::ItemVnum vnum) noexcept {
        if (!m_active_exchange) {
            return EterBase::MakeError("Brak aktywnej wymiany");
        }
        return m_active_exchange->AddItem(player, slot, vnum);
    }

    /**
     * @brief Deklaruje sume zlota, ktora gracz chce przekazac.
     */
    EterBase::VoidResult<std::string_view> AddExchangeGold(EterBase::EntityId player, Client::Gameplay::Gold amount) noexcept {
        if (!m_active_exchange) {
            return EterBase::MakeError("Brak aktywnej wymiany");
        }
        return m_active_exchange->AddGold(player, amount);
    }

    /**
     * @brief Blokuje wymiane dla gracza (zatwierdza przedmioty i zloto).
     */
    EterBase::VoidResult<std::string_view> LockExchange(EterBase::EntityId player) noexcept {
        if (!m_active_exchange) {
            return EterBase::MakeError("Brak aktywnej wymiany");
        }
        return m_active_exchange->Lock(player);
    }

    /**
     * @brief Ostatecznie akceptuje wymiane dla gracza (wymaga wczesniejszego zablokowania).
     */
    EterBase::VoidResult<std::string_view> AcceptExchange(EterBase::EntityId player) noexcept {
        if (!m_active_exchange) {
            return EterBase::MakeError("Brak aktywnej wymiany");
        }
        
        auto res = m_active_exchange->Accept(player);
        if (res.has_value() && m_active_exchange->GetState() == Client::Gameplay::ExchangeState::Accept) {
            // Wymiana zakonczona sukcesem, czyscimy stan
            m_active_exchange.reset();
        }
        return res;
    }

    // ========================================================================
    // API Sklepu NPC (NPC Shop)
    // ========================================================================

    /**
     * @brief Otwiera sklep NPC.
     */
    EterBase::VoidResult<std::string_view> OpenShop(std::unique_ptr<Client::Gameplay::NpcShop> shop) noexcept {
        if (m_active_shop) {
            return EterBase::MakeError("Inny sklep jest juz otwarty");
        }
        if (!shop) {
            return EterBase::MakeError("Przekazano pusty wskaznik na sklep");
        }
        
        m_active_shop = std::move(shop);
        return {};
    }

    /**
     * @brief Zamyka otwarty sklep.
     */
    EterBase::VoidResult<std::string_view> CloseShop() noexcept {
        if (!m_active_shop) {
            return EterBase::MakeError("Brak otwartego sklepu do zamkniecia");
        }
        
        m_active_shop.reset();
        return {};
    }

    /**
     * @brief Kupuje przedmiot ze sklepu.
     */
    EterBase::Result<Client::Gameplay::Gold, std::string_view> BuyFromShop(EterBase::ItemVnum vnum, uint8_t count, Client::Gameplay::Gold current_gold) const noexcept {
        if (!m_active_shop) {
            return EterBase::MakeError("Brak otwartego sklepu");
        }
        return m_active_shop->BuyItem(vnum, count, current_gold);
    }

    /**
     * @brief Sprzedaje przedmiot do sklepu.
     */
    EterBase::Result<Client::Gameplay::Gold, std::string_view> SellToShop(EterBase::ItemVnum vnum, uint8_t count) const noexcept {
        if (!m_active_shop) {
            return EterBase::MakeError("Brak otwartego sklepu");
        }
        return m_active_shop->SellItem(vnum, count);
    }

    /**
     * @brief Sprawdza czy wymiana jest aktualnie aktywna.
     */
    [[nodiscard]] bool IsExchangeActive() const noexcept {
        return m_active_exchange != nullptr;
    }

    /**
     * @brief Sprawdza czy sklep jest aktualnie otwarty.
     */
    [[nodiscard]] bool IsShopActive() const noexcept {
        return m_active_shop != nullptr;
    }

private:
    std::unique_ptr<Client::Gameplay::PlayerExchange> m_active_exchange{nullptr};
    std::unique_ptr<Client::Gameplay::NpcShop> m_active_shop{nullptr};
};

} // namespace Client::Domain
