#include "TradeDomain.h"
#include "EterBase/ModernLogger.h"

namespace Client::Gameplay {

// ============================================================================
// NpcShop
// ============================================================================

void NpcShop::RegisterItem(const ShopItem& item) {
    m_items[item.vnum] = item;
    EterBase::ModernLogger::Info("Registered shop item: Vnum={}", item.vnum.get());
}

std::optional<ShopItem> NpcShop::GetItem(EterBase::ItemVnum vnum) const {
    auto it = m_items.find(vnum);
    if (it != m_items.end()) {
        return it->second;
    }
    return std::nullopt;
}

EterBase::Result<Gold, std::string_view> NpcShop::BuyItem(EterBase::ItemVnum vnum, uint8_t count, Gold current_gold) const {
    auto item_opt = GetItem(vnum);
    if (!item_opt) {
        EterBase::ModernLogger::Error("BuyItem failed: Item Vnum={} not found in shop.", vnum.get());
        return EterBase::MakeError("Item not found in shop");
    }

    if (count == 0) {
         return EterBase::MakeError("Count must be greater than 0");
    }

    uint64_t total_price = static_cast<uint64_t>(item_opt->buy_price.get()) * count;
    
    if (current_gold.get() < total_price) {
        EterBase::ModernLogger::Error("BuyItem failed: Insufficient gold. Need {}, have {}.", total_price, current_gold.get());
        return EterBase::MakeError("Insufficient gold");
    }

    return Gold(current_gold.get() - total_price);
}

EterBase::Result<Gold, std::string_view> NpcShop::SellItem(EterBase::ItemVnum vnum, uint8_t count) const {
    auto item_opt = GetItem(vnum);
    if (!item_opt) {
        EterBase::ModernLogger::Error("SellItem failed: Item Vnum={} not found in shop.", vnum.get());
        return EterBase::MakeError("Item not found in shop");
    }

    if (count == 0) {
         return EterBase::MakeError("Count must be greater than 0");
    }

    uint64_t total_gain = static_cast<uint64_t>(item_opt->sell_price.get()) * count;
    return Gold(total_gain);
}


// ============================================================================
// PlayerExchange
// ============================================================================

PlayerExchange::PlayerExchange(EterBase::EntityId initiator, EterBase::EntityId target) {
    m_initiator.id = initiator;
    m_target.id = target;
    m_state = ExchangeState::Start;
    EterBase::ModernLogger::Info("Exchange started between {} and {}", initiator.get(), target.get());
}

ExchangeParticipant* PlayerExchange::GetParticipantMutable(EterBase::EntityId player) {
    if (m_initiator.id == player) return &m_initiator;
    if (m_target.id == player) return &m_target;
    return nullptr;
}

const ExchangeParticipant* PlayerExchange::GetParticipant(EterBase::EntityId player) const {
    if (m_initiator.id == player) return &m_initiator;
    if (m_target.id == player) return &m_target;
    return nullptr;
}

EterBase::VoidResult<std::string_view> PlayerExchange::AddItem(EterBase::EntityId player, EterBase::ItemSlot slot, EterBase::ItemVnum vnum) {
    if (m_state == ExchangeState::Cancel || m_state == ExchangeState::Accept) {
        return EterBase::MakeError("Exchange is already finished");
    }

    ExchangeParticipant* p = GetParticipantMutable(player);
    if (!p) {
        return EterBase::MakeError("Player not found in exchange");
    }

    if (p->is_locked) {
        EterBase::ModernLogger::Error("AddItem failed: Player {} has already locked the exchange.", player.get());
        return EterBase::MakeError("Exchange is locked");
    }

    p->items.emplace_back(slot, vnum);
    m_state = ExchangeState::AddItem;
    
    EterBase::ModernLogger::Info("Player {} added item {} from slot {}", player.get(), vnum.get(), slot.get());
    return {};
}

EterBase::VoidResult<std::string_view> PlayerExchange::AddGold(EterBase::EntityId player, Gold amount) {
    if (m_state == ExchangeState::Cancel || m_state == ExchangeState::Accept) {
        return EterBase::MakeError("Exchange is already finished");
    }

    ExchangeParticipant* p = GetParticipantMutable(player);
    if (!p) {
        return EterBase::MakeError("Player not found in exchange");
    }

    if (p->is_locked) {
        EterBase::ModernLogger::Error("AddGold failed: Player {} has already locked the exchange.", player.get());
        return EterBase::MakeError("Exchange is locked");
    }

    p->gold = amount;
    m_state = ExchangeState::AddGold;
    
    EterBase::ModernLogger::Info("Player {} set gold to {}", player.get(), amount.get());
    return {};
}

EterBase::VoidResult<std::string_view> PlayerExchange::Lock(EterBase::EntityId player) {
    if (m_state == ExchangeState::Cancel || m_state == ExchangeState::Accept) {
        return EterBase::MakeError("Exchange is already finished");
    }

    ExchangeParticipant* p = GetParticipantMutable(player);
    if (!p) {
        return EterBase::MakeError("Player not found in exchange");
    }

    p->is_locked = true;
    m_state = ExchangeState::Lock;
    
    EterBase::ModernLogger::Info("Player {} locked the exchange", player.get());
    return {};
}

EterBase::VoidResult<std::string_view> PlayerExchange::Accept(EterBase::EntityId player) {
    if (m_state == ExchangeState::Cancel) {
        return EterBase::MakeError("Exchange is already cancelled");
    }

    ExchangeParticipant* p = GetParticipantMutable(player);
    if (!p) {
        return EterBase::MakeError("Player not found in exchange");
    }

    if (!p->is_locked) {
        EterBase::ModernLogger::Error("Accept failed: Player {} must lock before accepting.", player.get());
        return EterBase::MakeError("Must lock before accepting");
    }

    p->is_accepted = true;
    
    if (m_initiator.is_accepted && m_target.is_accepted) {
        m_state = ExchangeState::Accept;
        EterBase::ModernLogger::Info("Exchange accepted by both players");
    } else {
        EterBase::ModernLogger::Info("Player {} accepted the exchange", player.get());
    }

    return {};
}

EterBase::VoidResult<std::string_view> PlayerExchange::Cancel() {
    if (m_state == ExchangeState::Cancel || m_state == ExchangeState::Accept) {
         return EterBase::MakeError("Exchange is already finished");
    }

    m_state = ExchangeState::Cancel;
    EterBase::ModernLogger::Info("Exchange cancelled");
    return {};
}

// ============================================================================
// SafeBox
// ============================================================================

SafeBox::SafeBox(uint16_t max_slots) : m_max_slots(max_slots) {
    EterBase::ModernLogger::Info("SafeBox created with {} slots", max_slots);
}

EterBase::VoidResult<std::string_view> SafeBox::SetItem(EterBase::ItemSlot slot, const SafeBoxItem& item) {
    if (slot.get() >= m_max_slots) {
        EterBase::ModernLogger::Error("SafeBox SetItem failed: Slot {} is out of bounds (max {}).", slot.get(), m_max_slots);
        return EterBase::MakeError("Slot out of bounds");
    }

    m_slots[slot] = item;
    return {};
}

EterBase::VoidResult<std::string_view> SafeBox::RemoveItem(EterBase::ItemSlot slot) {
    if (slot.get() >= m_max_slots) {
        EterBase::ModernLogger::Error("SafeBox RemoveItem failed: Slot {} is out of bounds (max {}).", slot.get(), m_max_slots);
        return EterBase::MakeError("Slot out of bounds");
    }

    m_slots.erase(slot);
    return {};
}

std::optional<SafeBoxItem> SafeBox::GetItem(EterBase::ItemSlot slot) const {
    if (slot.get() >= m_max_slots) {
        return std::nullopt;
    }

    auto it = m_slots.find(slot);
    if (it != m_slots.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

} // namespace Client::Gameplay
