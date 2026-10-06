#include "../../StdAfx.h"
/**
 * @file SendMovePacket.cpp
 * @brief C++23 Modern Implementation of Movement Packet Sender (Metin2 Standard 2026).
 */

#include "../../Packet.h"
#include "../../PythonBackground.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Timer.h"

#include <cmath>
#include <cstdint>
#include <vector>
#include <span>

namespace UserInterface::Network::Senders {

/**
 * @brief Formats and dispatches a standard movement packet via EventBus.
 * 
 * Takes local coordinates, transforms them into global coordinates, applies
 * correct server-side rotation logic (0-72 segments mapped to rotation/5.0),
 * and triggers a `NetworkPacketReceivedEvent` across the `EventBus` to notify
 * the network layer. This guarantees 100% decoupling from GUI and specific handlers.
 * 
 * @param position The local pixel position where the character intends to move.
 * @param rotation The facing direction of the character in degrees.
 * @param func The movement function state (e.g., walk, run, wait).
 * @param arg Additional argument required by specific movement states.
 * 
 * @return EterBase::PacketResult<void> Success or error context (std::expected).
 */
EterBase::PacketResult<void> SendMovePacket(const TPixelPosition& position, float rotation, uint8_t func, uint8_t arg)
{
    // Normalize rotation strictly within [0, 360)
    if (rotation < 0.0f) {
        rotation = 360.0f + std::fmod(rotation, 360.0f);
    } else if (rotation >= 360.0f) {
        rotation = std::fmod(rotation, 360.0f);
    }

    // Convert exact floats to standard 32-bit signed ints required by server
    int32_t x = static_cast<int32_t>(position.x);
    int32_t y = static_cast<int32_t>(position.y);

    // Apply global base coordinate offset
    CPythonBackground::Instance().LocalPositionToGlobalPosition(x, y);

    // Prepare packet structure (guaranteed no padding due to #pragma pack in Packet.h)
    TPacketCGMove movePacket{};
    movePacket.header = CG::MOVE;
    movePacket.length = sizeof(TPacketCGMove);
    movePacket.bFunc  = func;
    movePacket.bArg   = arg;
    // Rotation is compressed as segments (0-72) -> 360 / 5 = 72
    movePacket.bRot   = static_cast<uint8_t>(rotation / 5.0f);
    movePacket.lX     = x;
    movePacket.lY     = y;
    movePacket.dwTime = ELTimer_GetServerMSec();

    // Serialize to standard buffer and dispatch
    const auto* rawData = reinterpret_cast<const uint8_t*>(&movePacket);
    std::span<const uint8_t> payload(rawData, sizeof(movePacket));

    try {
        Core::EventBus::GetInstance().Publish(Core::NetworkPacketReceivedEvent(static_cast<uint8_t>(CG::MOVE), payload));
    } catch (const std::exception& e) {
        EterBase::ModernLogger::Error("Failed to publish move packet event: {}", e.what());
        return EterBase::MakeError(EterBase::PacketError::SessionClosed);
    }

    EterBase::ModernLogger::Debug("SendMovePacket: Dispatched MOVE event (X: {}, Y: {}, Rot: {})", x, y, rotation);

    return {};
}

} // namespace UserInterface::Network::Senders
