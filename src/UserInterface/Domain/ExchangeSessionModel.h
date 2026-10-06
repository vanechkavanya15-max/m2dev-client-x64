#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <optional>
#include <expected>
#include <format>
#include <string_view>

#include "../GameType.h"
#include "../Core/EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/Singleton.h"

/**
 * @file ExchangeSessionModel.h
 * @brief Modern C++23 Domain Model for the Exchange (Trading) Session.
 * 
 * Replaces the archaic CPythonExchange stateful singleton. This model is
 * completely decoupled from GUI logic and utilizes strong types, std::expected,
 * and the Core::EventBus for pure event-driven architecture.
 */

namespace UserInterface::Domain {

// ============================================================================
// Event Definitions
// ============================================================================

/**
 * @brief Event triggered when an exchange session successfully starts.
 */
struct ExchangeStartedEvent : public Core::IEvent {
    std::string selfName;
    std::string targetName;

    ExchangeStartedEvent(std::string self, std::string target)
        : selfName(std::move(self)), targetName(std::move(target)) {}
};

/**
 * @brief Event triggered when the current exchange session ends or is aborted.
 */
struct ExchangeEndedEvent : public Core::IEvent {
    ExchangeEndedEvent() = default;
};

/**
 * @brief Event triggered when an item in the exchange grid is added, updated, or removed.
 */
struct ExchangeItemUpdatedEvent : public Core::IEvent {
    bool isSelf; ///< True if the update is for the local player's side.
    EterBase::ItemSlot slot; ///< The grid slot index.

    ExchangeItemUpdatedEvent(bool isSelf, EterBase::ItemSlot slot)
        : isSelf(isSelf), slot(slot) {}
};

/**
 * @brief Event triggered when the offered currency (Elk/Yang) amount changes.
 */
struct ExchangeElkUpdatedEvent : public Core::IEvent {
    bool isSelf;
    uint32_t amount;

    ExchangeElkUpdatedEvent(bool isSelf, uint32_t amount)
        : isSelf(isSelf), amount(amount) {}
};

/**
 * @brief Event triggered when a player's readiness (Accept) state changes.
 */
struct ExchangeAcceptUpdatedEvent : public Core::IEvent {
    bool isSelf;
    bool isAccepted;

    ExchangeAcceptUpdatedEvent(bool isSelf, bool isAccepted)
        : isSelf(isSelf), isAccepted(isAccepted) {}
};

// ============================================================================
// Domain Data Structures
// ============================================================================

/**
 * @brief Represents a single item placed in the exchange grid.
 */
struct ExchangeItemData {
    EterBase::ItemVnum vnum;
    uint8_t count;
    std::array<int32_t, ITEM_SOCKET_SLOT_MAX_NUM> sockets;
    std::array<TPlayerItemAttribute, ITEM_ATTRIBUTE_SLOT_MAX_NUM> attributes;

    ExchangeItemData() : vnum(0), count(0) {
        sockets.fill(0);
        for (auto& attr : attributes) {
            attr.bType = 0;
            attr.sValue = 0;
        }
    }

    ExchangeItemData(EterBase::ItemVnum v, uint8_t c) : vnum(v), count(c) {
        sockets.fill(0);
        for (auto& attr : attributes) {
            attr.bType = 0;
            attr.sValue = 0;
        }
    }
};

/**
 * @brief Represents the current state of one party (self or target) in the exchange.
 */
struct ExchangeSide {
    std::string name;
    uint32_t elk = 0;
    bool isAccepted = false;
    
    // We use CPythonExchange::EXCHANGE_ITEM_MAX_NUM = 12
    static constexpr size_t MAX_ITEMS = 12;
    std::array<std::optional<ExchangeItemData>, MAX_ITEMS> items;

    void Clear() {
        name.clear();
        elk = 0;
        isAccepted = false;
        items.fill(std::nullopt);
    }
};

// ============================================================================
// Core Domain Model
// ============================================================================

/**
 * @brief Manages the state of a player-to-player trade session.
 */
class ExchangeSessionModel : public CSingleton<ExchangeSessionModel> {
public:
    ExchangeSessionModel() = default;
    ~ExchangeSessionModel() = default;

    /**
     * @brief Resets the exchange session to its default empty state.
     */
    void Clear() {
        m_isTrading = false;
        m_self.Clear();
        m_target.Clear();
    }

    /**
     * @brief Initiates a new exchange session.
     * @param selfName Name of the local player.
     * @param targetName Name of the trading partner.
     * @return Success or EntityError if an exchange is already active.
     */
    std::expected<void, EterBase::EntityError> Start(std::string selfName, std::string targetName) {
        if (m_isTrading) {
            return std::unexpected(EterBase::EntityError::AlreadyExists);
        }

        Clear();
        m_isTrading = true;
        m_self.name = std::move(selfName);
        m_target.name = std::move(targetName);

        Core::EventBus::GetInstance().Publish(ExchangeStartedEvent(m_self.name, m_target.name));
        return {};
    }

    /**
     * @brief Ends the current exchange session and clears state.
     */
    void End() {
        if (m_isTrading) {
            Clear();
            Core::EventBus::GetInstance().Publish(ExchangeEndedEvent());
        }
    }

    /**
     * @brief Checks if an exchange session is currently active.
     * @return True if trading, false otherwise.
     */
    [[nodiscard]] bool IsTrading() const noexcept {
        return m_isTrading;
    }

    // --- Accessors & Mutators for Self ---

    /**
     * @brief Sets the Elk (Yang) offered by the local player.
     * @param amount The new currency amount.
     */
    void SetElkToSelf(uint32_t amount) {
        m_self.elk = amount;
        Core::EventBus::GetInstance().Publish(ExchangeElkUpdatedEvent(true, amount));
    }

    /**
     * @brief Sets the acceptance state of the local player.
     * @param accept True to accept, false to cancel acceptance.
     */
    void SetAcceptToSelf(bool accept) {
        m_self.isAccepted = accept;
        Core::EventBus::GetInstance().Publish(ExchangeAcceptUpdatedEvent(true, accept));
    }

    /**
     * @brief Adds or updates an item in the local player's exchange grid.
     * @param slot The grid slot index (0 to 11).
     * @param vnum The item's VNUM.
     * @param count The item's quantity.
     * @param sockets Array of Metin stone sockets.
     * @param attributes Array of magical attributes.
     * @return Success or EntityError if the slot is out of bounds.
     */
    std::expected<void, EterBase::EntityError> SetItemToSelf(
        EterBase::ItemSlot slot, EterBase::ItemVnum vnum, uint8_t count,
        const std::array<int32_t, ITEM_SOCKET_SLOT_MAX_NUM>& sockets,
        const std::array<TPlayerItemAttribute, ITEM_ATTRIBUTE_SLOT_MAX_NUM>& attributes) 
    {
        if (slot.get() >= ExchangeSide::MAX_ITEMS) {
            return std::unexpected(EterBase::EntityError::OutOfRange);
        }

        ExchangeItemData data(vnum, count);
        data.sockets = sockets;
        data.attributes = attributes;
        m_self.items[slot.get()] = data;

        Core::EventBus::GetInstance().Publish(ExchangeItemUpdatedEvent(true, slot));
        return {};
    }

    /**
     * @brief Removes an item from the local player's exchange grid.
     * @param slot The grid slot index.
     * @return Success or EntityError if the slot is out of bounds.
     */
    std::expected<void, EterBase::EntityError> DelItemOfSelf(EterBase::ItemSlot slot) {
        if (slot.get() >= ExchangeSide::MAX_ITEMS) {
            return std::unexpected(EterBase::EntityError::OutOfRange);
        }

        m_self.items[slot.get()] = std::nullopt;
        Core::EventBus::GetInstance().Publish(ExchangeItemUpdatedEvent(true, slot));
        return {};
    }

    // --- Accessors & Mutators for Target (Victim) ---

    /**
     * @brief Sets the Elk (Yang) offered by the target player.
     * @param amount The new currency amount.
     */
    void SetElkToTarget(uint32_t amount) {
        m_target.elk = amount;
        Core::EventBus::GetInstance().Publish(ExchangeElkUpdatedEvent(false, amount));
    }

    /**
     * @brief Sets the acceptance state of the target player.
     * @param accept True if target accepts the trade.
     */
    void SetAcceptToTarget(bool accept) {
        m_target.isAccepted = accept;
        Core::EventBus::GetInstance().Publish(ExchangeAcceptUpdatedEvent(false, accept));
    }

    /**
     * @brief Adds or updates an item in the target player's exchange grid.
     * @param slot The grid slot index.
     * @param vnum The item's VNUM.
     * @param count The item's quantity.
     * @param sockets Array of Metin stone sockets.
     * @param attributes Array of magical attributes.
     * @return Success or EntityError if the slot is out of bounds.
     */
    std::expected<void, EterBase::EntityError> SetItemToTarget(
        EterBase::ItemSlot slot, EterBase::ItemVnum vnum, uint8_t count,
        const std::array<int32_t, ITEM_SOCKET_SLOT_MAX_NUM>& sockets,
        const std::array<TPlayerItemAttribute, ITEM_ATTRIBUTE_SLOT_MAX_NUM>& attributes) 
    {
        if (slot.get() >= ExchangeSide::MAX_ITEMS) {
            return std::unexpected(EterBase::EntityError::OutOfRange);
        }

        ExchangeItemData data(vnum, count);
        data.sockets = sockets;
        data.attributes = attributes;
        m_target.items[slot.get()] = data;

        Core::EventBus::GetInstance().Publish(ExchangeItemUpdatedEvent(false, slot));
        return {};
    }

    /**
     * @brief Removes an item from the target player's exchange grid.
     * @param slot The grid slot index.
     * @return Success or EntityError if the slot is out of bounds.
     */
    std::expected<void, EterBase::EntityError> DelItemOfTarget(EterBase::ItemSlot slot) {
        if (slot.get() >= ExchangeSide::MAX_ITEMS) {
            return std::unexpected(EterBase::EntityError::OutOfRange);
        }

        m_target.items[slot.get()] = std::nullopt;
        Core::EventBus::GetInstance().Publish(ExchangeItemUpdatedEvent(false, slot));
        return {};
    }

    // --- Read-only Data Getters ---

    [[nodiscard]] const ExchangeSide& GetSelf() const noexcept { return m_self; }
    [[nodiscard]] const ExchangeSide& GetTarget() const noexcept { return m_target; }

private:
    bool m_isTrading = false;
    ExchangeSide m_self;
    ExchangeSide m_target;
};

} // namespace UserInterface::Domain
