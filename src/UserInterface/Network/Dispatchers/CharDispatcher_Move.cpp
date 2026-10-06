#include "../../StdAfx.h"
#include "../../Packet.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "Core/EventBus.h"

#include <span>
#include <cstring>
#include <cstdint>

namespace UserInterface::Network::Dispatchers {

/**
 * @brief Zero-conflict event emitted when a character move packet is received.
 * Decouples the network dispatcher from the UI and logic systems.
 */
struct CharacterMovedEvent : public Core::IEvent {
    EterBase::EntityId entityId;
    int32_t x;
    int32_t y;
    uint8_t rotation;
    uint8_t func;
    uint8_t arg;
    uint32_t time;
    uint32_t duration;

    CharacterMovedEvent(EterBase::EntityId entityId, int32_t x, int32_t y, uint8_t rot, uint8_t func, uint8_t arg, uint32_t time, uint32_t duration)
        : entityId(entityId), x(x), y(y), rotation(rot), func(func), arg(arg), time(time), duration(duration) {}
};

class CharDispatcher_Move {
public:
    /**
     * @brief Dispatches the character movement packet and emits a CharacterMovedEvent.
     * @param payload The binary payload of the network packet.
     * @return PacketResult<void> representing success or error.
     */
    static EterBase::PacketResult<void> Dispatch(std::span<const uint8_t> payload) {
#pragma pack(push, 1)
        struct LocalPacketGCMove {
            uint16_t header;
            uint16_t length;
            uint8_t  bFunc;
            uint8_t  bArg;
            uint8_t  bRot;
            uint32_t dwVID;
            int32_t  lX;
            int32_t  lY;
            uint32_t dwTime;
            uint32_t dwDuration;
        };
#pragma pack(pop)

        if (payload.size() < sizeof(LocalPacketGCMove)) {
            EterBase::ModernLogger::Error("CharDispatcher_Move: Payload too small. Expected: {}, Got: {}", 
                                          sizeof(LocalPacketGCMove), payload.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        LocalPacketGCMove packet{};
        std::memcpy(&packet, payload.data(), sizeof(LocalPacketGCMove));

        EterBase::EntityId entityId{packet.dwVID};

        EterBase::ModernLogger::Debug(
            "CharDispatcher_Move: Dispatching move for VID: {}, X: {}, Y: {}, Rot: {}, Func: {}, Arg: {}, Time: {}, Duration: {}", 
            entityId.get(), packet.lX, packet.lY, packet.bRot, packet.bFunc, packet.bArg, packet.dwTime, packet.dwDuration
        );

        CharacterMovedEvent event{
            entityId,
            packet.lX,
            packet.lY,
            packet.bRot,
            packet.bFunc,
            packet.bArg,
            packet.dwTime,
            packet.dwDuration
        };

        Core::EventBus::GetInstance().Publish(event);

        return {};
    }
};

} // namespace UserInterface::Network::Dispatchers
