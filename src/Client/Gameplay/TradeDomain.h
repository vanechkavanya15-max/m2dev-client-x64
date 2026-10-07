#pragma once

#include <vector>
#include <unordered_map>
#include <optional>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace Client::Gameplay {

struct GoldTag {};
struct PriceTag {};

using Gold = EterBase::StrongType<GoldTag, uint64_t, 0>;
using Price = EterBase::StrongType<PriceTag, uint64_t, 0>;

// ============================================================================
// NpcShop
// ============================================================================

struct ShopItem {
    EterBase::ItemVnum vnum;
    uint8_t count;
    Price buy_price;
    Price sell_price;
};

class NpcShop {
public:
    void RegisterItem(const ShopItem& item);
    std::optional<ShopItem> GetItem(EterBase::ItemVnum vnum) const;
    
    EterBase::Result<Gold, std::string_view> BuyItem(EterBase::ItemVnum vnum, uint8_t count, Gold current_gold) const;
    EterBase::Result<Gold, std::string_view> SellItem(EterBase::ItemVnum vnum, uint8_t count) const;

private:
    std::unordered_map<EterBase::ItemVnum, ShopItem> m_items;
};

// ============================================================================
// PlayerExchange
// ============================================================================

enum class ExchangeState {
    Start,
    AddItem,
    AddGold,
    Lock,
    Accept,
    Cancel
};

struct ExchangeParticipant {
    EterBase::EntityId id;
    Gold gold{0};
    std::vector<std::pair<EterBase::ItemSlot, EterBase::ItemVnum>> items;
    bool is_locked{false};
    bool is_accepted{false};
};

class PlayerExchange {
public:
    PlayerExchange(EterBase::EntityId initiator, EterBase::EntityId target);

    EterBase::VoidResult<std::string_view> AddItem(EterBase::EntityId player, EterBase::ItemSlot slot, EterBase::ItemVnum vnum);
    EterBase::VoidResult<std::string_view> AddGold(EterBase::EntityId player, Gold amount);
    EterBase::VoidResult<std::string_view> Lock(EterBase::EntityId player);
    EterBase::VoidResult<std::string_view> Accept(EterBase::EntityId player);
    EterBase::VoidResult<std::string_view> Cancel();

    ExchangeState GetState() const { return m_state; }
    const ExchangeParticipant* GetParticipant(EterBase::EntityId player) const;

private:
    ExchangeState m_state{ExchangeState::Start};
    ExchangeParticipant m_initiator;
    ExchangeParticipant m_target;

    ExchangeParticipant* GetParticipantMutable(EterBase::EntityId player);
};

// ============================================================================
// SafeBox
// ============================================================================

struct SafeBoxItem {
    EterBase::ItemVnum vnum;
    uint8_t count;
};

class SafeBox {
public:
    explicit SafeBox(uint16_t max_slots);

    EterBase::VoidResult<std::string_view> SetItem(EterBase::ItemSlot slot, const SafeBoxItem& item);
    EterBase::VoidResult<std::string_view> RemoveItem(EterBase::ItemSlot slot);
    std::optional<SafeBoxItem> GetItem(EterBase::ItemSlot slot) const;

private:
    uint16_t m_max_slots;
    std::unordered_map<EterBase::ItemSlot, SafeBoxItem> m_slots;
};

} // namespace Client::Gameplay
