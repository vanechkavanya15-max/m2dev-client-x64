#pragma once

#include <cstdint>
#include <span>
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
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

template<typename HandlerFn>
class FunctionalPacketHandler final : public IPacketHandler {
public:
    FunctionalPacketHandler(uint16_t minSize, bool dynamicSize, HandlerFn fn)
        : m_minSize(minSize), m_dynamicSize(dynamicSize), m_fn(std::move(fn)) {}

    [[nodiscard]] EterBase::PacketResult<void> Handle(std::span<const uint8_t> payload) override {
        return m_fn(payload);
    }
    [[nodiscard]] uint16_t GetExpectedSize() const override { return m_minSize; }
    [[nodiscard]] bool IsDynamicSize() const override { return m_dynamicSize; }

private:
    uint16_t m_minSize;
    bool m_dynamicSize;
    HandlerFn m_fn;
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

    static ModernPacketDispatcher& Instance() noexcept;

    void RegisterHandler(uint16_t opcode, IPacketHandler* handler);
    void RegisterHandler(uint8_t opcode, IPacketHandler* handler);
    void UnregisterHandler(uint16_t opcode);
    void UnregisterHandler(uint8_t opcode);
    [[nodiscard]] bool HasHandler(uint16_t opcode) const noexcept;

    void RegisterOwnedHandler(uint16_t opcode, std::unique_ptr<IPacketHandler> handler);
    void RegisterOwnedHandler(uint8_t opcode, std::unique_ptr<IPacketHandler> handler);

    template<typename HandlerFn>
    void RegisterFunctionHandler(uint16_t opcode, uint16_t minSize, bool dynamicSize, HandlerFn fn) {
        RegisterOwnedHandler(opcode, std::make_unique<FunctionalPacketHandler<HandlerFn>>(minSize, dynamicSize, std::move(fn)));
    }

    [[nodiscard]] EterBase::PacketResult<void> Dispatch(uint16_t opcode, std::span<const uint8_t> payload);
    [[nodiscard]] EterBase::PacketResult<void> Dispatch(uint8_t opcode, std::span<const uint8_t> payload);

    void Clear() noexcept;

private:
    std::array<IPacketHandler*, 256> m_handlers;
    std::unordered_map<uint16_t, IPacketHandler*> m_extendedHandlers;
    std::vector<std::unique_ptr<IPacketHandler>> m_ownedHandlers;
};

} // namespace Client::Network
