#include "PyTradeEventAdapter.h"

namespace Client::Bridge {

PyTradeEventAdapter& PyTradeEventAdapter::Instance() noexcept {
    static PyTradeEventAdapter instance;
    return instance;
}

void PyTradeEventAdapter::Initialize() noexcept {
    m_initialized = true;
}

void PyTradeEventAdapter::Shutdown() noexcept {
    m_initialized = false;
}

[[nodiscard]] Client::Core::Result<void, TradeAdapterError> PyTradeEventAdapter::OnExchangeStart(uint32_t targetVid) noexcept {
    if (!m_initialized) {
        return std::unexpected(TradeAdapterError::NotInitialized);
    }
    if (targetVid == 0) {
        return std::unexpected(TradeAdapterError::InvalidTarget);
    }
    EterBase::EventBus::Instance().Publish(ExchangeStartEvent{targetVid});
    return {};
}

[[nodiscard]] Client::Core::Result<void, TradeAdapterError> PyTradeEventAdapter::OnExchangeItemAdded(uint8_t windowType, uint16_t pos, uint32_t vnum, uint8_t count) noexcept {
    if (!m_initialized) {
        return std::unexpected(TradeAdapterError::NotInitialized);
    }
    if (vnum == 0 || count == 0) {
        return std::unexpected(TradeAdapterError::InvalidItem);
    }
    // Simple bounds check for pos if needed, assuming 0xFFFF is max
    if (pos == 0xFFFF) {
        return std::unexpected(TradeAdapterError::InvalidSlot);
    }
    EterBase::EventBus::Instance().Publish(ExchangeItemAddedEvent{windowType, pos, vnum, count});
    return {};
}

[[nodiscard]] Client::Core::Result<void, TradeAdapterError> PyTradeEventAdapter::OnExchangeGoldAdded(int64_t gold) noexcept {
    if (!m_initialized) {
        return std::unexpected(TradeAdapterError::NotInitialized);
    }
    if (gold <= 0) {
        return std::unexpected(TradeAdapterError::InvalidGoldAmount);
    }
    EterBase::EventBus::Instance().Publish(ExchangeGoldAddedEvent{gold});
    return {};
}

[[nodiscard]] Client::Core::Result<void, TradeAdapterError> PyTradeEventAdapter::OnExchangeAccept() noexcept {
    if (!m_initialized) {
        return std::unexpected(TradeAdapterError::NotInitialized);
    }
    EterBase::EventBus::Instance().Publish(ExchangeAcceptEvent{});
    return {};
}

[[nodiscard]] Client::Core::Result<void, TradeAdapterError> PyTradeEventAdapter::OnExchangeCancel() noexcept {
    if (!m_initialized) {
        return std::unexpected(TradeAdapterError::NotInitialized);
    }
    EterBase::EventBus::Instance().Publish(ExchangeCancelEvent{});
    return {};
}

[[nodiscard]] Client::Core::Result<void, TradeAdapterError> PyTradeEventAdapter::OnShopOpen(uint32_t npcVid) noexcept {
    if (!m_initialized) {
        return std::unexpected(TradeAdapterError::NotInitialized);
    }
    if (npcVid == 0) {
        return std::unexpected(TradeAdapterError::InvalidTarget);
    }
    EterBase::EventBus::Instance().Publish(ShopOpenEvent{npcVid});
    return {};
}

[[nodiscard]] Client::Core::Result<void, TradeAdapterError> PyTradeEventAdapter::OnShopClose() noexcept {
    if (!m_initialized) {
        return std::unexpected(TradeAdapterError::NotInitialized);
    }
    EterBase::EventBus::Instance().Publish(ShopCloseEvent{});
    return {};
}

[[nodiscard]] Client::Core::Result<void, TradeAdapterError> PyTradeEventAdapter::OnShopBuy(uint8_t pos) noexcept {
    if (!m_initialized) {
        return std::unexpected(TradeAdapterError::NotInitialized);
    }
    if (pos == 0xFF) {
        return std::unexpected(TradeAdapterError::InvalidSlot);
    }
    EterBase::EventBus::Instance().Publish(ShopBuyEvent{pos});
    return {};
}

[[nodiscard]] Client::Core::Result<void, TradeAdapterError> PyTradeEventAdapter::OnShopSell(uint8_t pos) noexcept {
    if (!m_initialized) {
        return std::unexpected(TradeAdapterError::NotInitialized);
    }
    if (pos == 0xFF) {
        return std::unexpected(TradeAdapterError::InvalidSlot);
    }
    EterBase::EventBus::Instance().Publish(ShopSellEvent{pos});
    return {};
}

} // namespace Client::Bridge
