#pragma once

#include <cstdint>
#include <span>
#include <array>
#include "../../EterBase/Result.h"
#include "../../EterBase/PacketResult.h"

namespace Client::Network {

class IPacketHandler {
public:
    virtual ~IPacketHandler() = default;

    [[nodiscard]] virtual EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) = 0;
    [[nodiscard]] virtual uint16_t GetExpectedSize() const = 0;
    [[nodiscard]] virtual bool IsDynamicSize() const = 0;
};

class ModernPacketDispatcher {
public:
    ModernPacketDispatcher();
    ~ModernPacketDispatcher() = default;

    // Prevent copying and moving (dispatcher is usually a singleton or bound to a specific session)
    ModernPacketDispatcher(const ModernPacketDispatcher&) = delete;
    ModernPacketDispatcher& operator=(const ModernPacketDispatcher&) = delete;
    ModernPacketDispatcher(ModernPacketDispatcher&&) = delete;
    ModernPacketDispatcher& operator=(ModernPacketDispatcher&&) = delete;

    void RegisterHandler(uint8_t opcode, IPacketHandler* handler);
    void UnregisterHandler(uint8_t opcode);

    [[nodiscard]] EterBase::PacketResult<void> Dispatch(uint8_t opcode, std::span<const uint8_t> payload);

private:
    std::array<IPacketHandler*, 256> m_handlers;
};

} // namespace Client::Network
