#pragma once

#include <cstdint>
#include <string_view>
#include <optional>
#include "Client/Core/Result.h"
#include "Client/Core/EventBus.h"
#include "EterBase/StrongTypes.h"

namespace Client::Bridge {

// Zdarzenia Handlu (Exchange)
struct ExchangeStartEvent : public EterBase::IEvent {
    uint32_t targetVid;
    explicit ExchangeStartEvent(uint32_t targetVid = 0) : targetVid(targetVid) {}
};

struct ExchangeItemAddedEvent : public EterBase::IEvent {
    uint8_t windowType;
    uint16_t pos;
    uint32_t vnum;
    uint8_t count;
    ExchangeItemAddedEvent(uint8_t windowType = 0, uint16_t pos = 0, uint32_t vnum = 0, uint8_t count = 0)
        : windowType(windowType), pos(pos), vnum(vnum), count(count) {}
};

struct ExchangeGoldAddedEvent : public EterBase::IEvent {
    int64_t gold;
    explicit ExchangeGoldAddedEvent(int64_t gold = 0) : gold(gold) {}
};

struct ExchangeAcceptEvent : public EterBase::IEvent {
    ExchangeAcceptEvent() = default;
};

struct ExchangeCancelEvent : public EterBase::IEvent {
    ExchangeCancelEvent() = default;
};

// Zdarzenia Sklepu (Shop)
struct ShopOpenEvent : public EterBase::IEvent {
    uint32_t npcVid;
    explicit ShopOpenEvent(uint32_t npcVid = 0) : npcVid(npcVid) {}
};

struct ShopCloseEvent : public EterBase::IEvent {
    ShopCloseEvent() = default;
};

struct ShopBuyEvent : public EterBase::IEvent {
    uint8_t pos;
    explicit ShopBuyEvent(uint8_t pos = 0) : pos(pos) {}
};

struct ShopSellEvent : public EterBase::IEvent {
    uint8_t pos;
    explicit ShopSellEvent(uint8_t pos = 0) : pos(pos) {}
};

// Bledy adaptera
enum class TradeAdapterError : uint8_t {
    None = 0,
    InvalidTarget,
    InvalidItem,
    InvalidGoldAmount,
    InvalidSlot,
    NotInitialized
};

[[nodiscard]] constexpr std::string_view to_string(TradeAdapterError err) noexcept {
    switch (err) {
        case TradeAdapterError::None: return "None";
        case TradeAdapterError::InvalidTarget: return "InvalidTarget";
        case TradeAdapterError::InvalidItem: return "InvalidItem";
        case TradeAdapterError::InvalidGoldAmount: return "InvalidGoldAmount";
        case TradeAdapterError::InvalidSlot: return "InvalidSlot";
        case TradeAdapterError::NotInitialized: return "NotInitialized";
        default: return "Unknown";
    }
}

/**
 * @brief Adapter Mostka Pythona do EventBusa dla zdarzen Handlu i Sklepu.
 */
class PyTradeEventAdapter {
public:
    static PyTradeEventAdapter& Instance() noexcept;

    void Initialize() noexcept;
    void Shutdown() noexcept;

    // Delegowanie zdarzen Exchange do EventBusa
    [[nodiscard]] Client::Core::Result<void, TradeAdapterError> OnExchangeStart(uint32_t targetVid) noexcept;
    [[nodiscard]] Client::Core::Result<void, TradeAdapterError> OnExchangeItemAdded(uint8_t windowType, uint16_t pos, uint32_t vnum, uint8_t count) noexcept;
    [[nodiscard]] Client::Core::Result<void, TradeAdapterError> OnExchangeGoldAdded(int64_t gold) noexcept;
    [[nodiscard]] Client::Core::Result<void, TradeAdapterError> OnExchangeAccept() noexcept;
    [[nodiscard]] Client::Core::Result<void, TradeAdapterError> OnExchangeCancel() noexcept;

    // Delegowanie zdarzen Shop do EventBusa
    [[nodiscard]] Client::Core::Result<void, TradeAdapterError> OnShopOpen(uint32_t npcVid) noexcept;
    [[nodiscard]] Client::Core::Result<void, TradeAdapterError> OnShopClose() noexcept;
    [[nodiscard]] Client::Core::Result<void, TradeAdapterError> OnShopBuy(uint8_t pos) noexcept;
    [[nodiscard]] Client::Core::Result<void, TradeAdapterError> OnShopSell(uint8_t pos) noexcept;

private:
    PyTradeEventAdapter() = default;
    ~PyTradeEventAdapter() = default;

    PyTradeEventAdapter(const PyTradeEventAdapter&) = delete;
    PyTradeEventAdapter& operator=(const PyTradeEventAdapter&) = delete;
    PyTradeEventAdapter(PyTradeEventAdapter&&) = delete;
    PyTradeEventAdapter& operator=(PyTradeEventAdapter&&) = delete;

    bool m_initialized{false};
};

} // namespace Client::Bridge
