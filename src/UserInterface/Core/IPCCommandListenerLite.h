#pragma once

#include <cstdint>
#include <span>
#include <optional>
#include <expected>
#include <cstring>
#include <format>
#include <memory>
#include <string_view>

#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace UserInterface::Core {

// ============================================================================
// Protocol Enumerations
// ============================================================================

/**
 * @brief Opcodes for the incoming IPC commands.
 */
enum class IpcOpcode : uint16_t {
    Goto = 0x1001,
    Attack = 0x1002,
    Loot = 0x1003
};

// ============================================================================
// IPC Data Structures (Packed)
// ============================================================================
#pragma pack(push, 1)

/**
 * @brief Universal IPC command header layout.
 */
struct IpcCommandHeader {
    uint16_t opcode;
    uint16_t length;
};

/**
 * @brief Payload for the Goto IPC command.
 */
struct IpcGotoPayload {
    int32_t x;
    int32_t y;
};

/**
 * @brief Payload for the Attack IPC command.
 */
struct IpcAttackPayload {
    EterBase::EntityId targetId;
};

/**
 * @brief Payload for the Loot IPC command.
 */
struct IpcLootPayload {
    EterBase::ItemVnum itemId;
};

#pragma pack(pop)

// ============================================================================
// Domain Events
// ============================================================================

/**
 * @brief Event published when an IPC Goto command is parsed.
 */
struct IpcGotoEvent : public IEvent {
    int32_t x;
    int32_t y;

    explicit IpcGotoEvent(int32_t x, int32_t y) : x(x), y(y) {}
};

/**
 * @brief Event published when an IPC Attack command is parsed.
 */
struct IpcAttackEvent : public IEvent {
    EterBase::EntityId targetId;

    explicit IpcAttackEvent(EterBase::EntityId targetId) : targetId(targetId) {}
};

/**
 * @brief Event published when an IPC Loot command is parsed.
 */
struct IpcLootEvent : public IEvent {
    EterBase::ItemVnum itemId;

    explicit IpcLootEvent(EterBase::ItemVnum itemId) : itemId(itemId) {}
};

// ============================================================================
// Command Listener Core
// ============================================================================

/**
 * @brief Headless, event-driven listener for IPC Commands.
 * 
 * Safely parses raw binary buffers using C++23 std::span and std::expected.
 * Publishes valid domain events via the EventBus without depending on UI code.
 */
class IPCCommandListenerLite {
public:
    IPCCommandListenerLite() = default;
    ~IPCCommandListenerLite() = default;

    /**
     * @brief Processes a raw byte buffer, decodes the IPC command, and publishes it.
     * @param buffer A span covering the incoming bytes.
     * @return PacketResult indicating success or a well-defined domain error.
     */
    EterBase::PacketResult<void> ProcessBuffer(std::span<const uint8_t> buffer) {
        return ExtractHeader(buffer)
            .and_then([this](const DecodedPacket& packet) {
                return DispatchPayload(packet.header, packet.payload);
            });
    }

private:
    struct DecodedPacket {
        IpcCommandHeader header;
        std::span<const uint8_t> payload;
    };

    /**
     * @brief Validates and extracts the command header from the span.
     * @param buffer Raw incoming buffer.
     * @return Decoded packet split into header and payload, or an error.
     */
    EterBase::PacketResult<DecodedPacket> ExtractHeader(std::span<const uint8_t> buffer) const {
        if (buffer.size() < sizeof(IpcCommandHeader)) {
            EterBase::ModernLogger::Warn("IPCCommandListenerLite: Buffer underflow while reading header");
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        IpcCommandHeader header;
        std::memcpy(&header, buffer.data(), sizeof(IpcCommandHeader));

        if (header.length < sizeof(IpcCommandHeader)) {
            EterBase::ModernLogger::Warn("IPCCommandListenerLite: Header length {} is smaller than minimum required {}", header.length, sizeof(IpcCommandHeader));
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        if (buffer.size() < header.length) {
            EterBase::ModernLogger::Warn("IPCCommandListenerLite: Malformed payload size. Expected: {}, Got: {}", header.length, buffer.size());
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        std::span<const uint8_t> payload = buffer.subspan(sizeof(IpcCommandHeader), header.length - sizeof(IpcCommandHeader));
        return DecodedPacket{header, payload};
    }

    /**
     * @brief Dispatches the payload to the specific parser based on opcode.
     * @param header The extracted header.
     * @param payload The raw payload slice.
     * @return PacketResult success or an error if decoding fails.
     */
    EterBase::PacketResult<void> DispatchPayload(const IpcCommandHeader& header, std::span<const uint8_t> payload) const {
        switch (static_cast<IpcOpcode>(header.opcode)) {
            case IpcOpcode::Goto:
                return HandleGoto(payload);
            case IpcOpcode::Attack:
                return HandleAttack(payload);
            case IpcOpcode::Loot:
                return HandleLoot(payload);
            default:
                EterBase::ModernLogger::Warn("IPCCommandListenerLite: Unknown opcode {}", header.opcode);
                return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }
    }

    /**
     * @brief Safe payload extractor helper returning optional.
     * @tparam T Payload structure type.
     * @param payload Raw payload bytes.
     * @return T on success, std::nullopt on size mismatch.
     */
    template <typename T>
    std::optional<T> ExtractPayload(std::span<const uint8_t> payload) const {
        if (payload.size() < sizeof(T)) {
            return std::nullopt;
        }
        T data;
        std::memcpy(&data, payload.data(), sizeof(T));
        return data;
    }

    EterBase::PacketResult<void> HandleGoto(std::span<const uint8_t> payload) const {
        auto result = ExtractPayload<IpcGotoPayload>(payload)
            .transform([](const IpcGotoPayload& data) {
                EterBase::ModernLogger::Info("IPCCommandListenerLite: Goto({}, {})", data.x, data.y);
                Core::EventBus::Instance().Publish(IpcGotoEvent{data.x, data.y});
                return EterBase::PacketResult<void>{};
            });
            
        return result.value_or(EterBase::MakeError(EterBase::PacketError::MalformedPayload));
    }

    EterBase::PacketResult<void> HandleAttack(std::span<const uint8_t> payload) const {
        auto result = ExtractPayload<IpcAttackPayload>(payload)
            .transform([](const IpcAttackPayload& data) {
                EterBase::ModernLogger::Info("IPCCommandListenerLite: Attack({})", data.targetId.value());
                Core::EventBus::Instance().Publish(IpcAttackEvent{data.targetId});
                return EterBase::PacketResult<void>{};
            });

        return result.value_or(EterBase::MakeError(EterBase::PacketError::MalformedPayload));
    }

    EterBase::PacketResult<void> HandleLoot(std::span<const uint8_t> payload) const {
        auto result = ExtractPayload<IpcLootPayload>(payload)
            .transform([](const IpcLootPayload& data) {
                EterBase::ModernLogger::Info("IPCCommandListenerLite: Loot({})", data.itemId.value());
                Core::EventBus::Instance().Publish(IpcLootEvent{data.itemId});
                return EterBase::PacketResult<void>{};
            });

        return result.value_or(EterBase::MakeError(EterBase::PacketError::MalformedPayload));
    }
};

} // namespace UserInterface::Core
