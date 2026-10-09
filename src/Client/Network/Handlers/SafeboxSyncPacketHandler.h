#pragma once

#include <span>
#include <cstdint>
#include "EterBase/Result.h"
#include "Client/Gameplay/InventoryDomain.h"
#include "Client/Core/EventBus.h"

namespace Client::Network::Handlers {

struct SafeboxSyncSizeEvent : public Client::Core::IEvent {
    uint8_t size;
    explicit SafeboxSyncSizeEvent(uint8_t size) : size(size) {}
};

struct SafeboxSyncWrongPasswordEvent : public Client::Core::IEvent {
    SafeboxSyncWrongPasswordEvent() = default;
};

struct SafeboxSyncMoneyChangeEvent : public Client::Core::IEvent {
    int32_t money;
    explicit SafeboxSyncMoneyChangeEvent(int32_t money) : money(money) {}
};

struct MallSyncOpenEvent : public Client::Core::IEvent {
    uint8_t size;
    explicit MallSyncOpenEvent(uint8_t size) : size(size) {}
};

/**
 * @brief Handles Safebox and Mall network packets (Safebox, Premium Safebox/Gold, and Item-Shop Warehouse).
 */
class SafeboxSyncPacketHandler {
public:
    static EterBase::PacketResult<void> HandleSafeboxSet(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain);
    static EterBase::PacketResult<void> HandleSafeboxDel(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain);
    static EterBase::PacketResult<void> HandleSafeboxSize(std::span<const uint8_t> buffer);
    static EterBase::PacketResult<void> HandleSafeboxWrongPassword(std::span<const uint8_t> buffer);
    static EterBase::PacketResult<void> HandleSafeboxMoneyChange(std::span<const uint8_t> buffer);

    static EterBase::PacketResult<void> HandleMallSet(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain);
    static EterBase::PacketResult<void> HandleMallDel(std::span<const uint8_t> buffer, Gameplay::InventoryDomain& inventoryDomain);
    static EterBase::PacketResult<void> HandleMallOpen(std::span<const uint8_t> buffer);
};

} // namespace Client::Network::Handlers
