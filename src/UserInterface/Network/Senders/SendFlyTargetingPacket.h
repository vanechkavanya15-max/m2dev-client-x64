#pragma once

#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include <cstdint>

class CPythonNetworkStream;

namespace UserInterface::Network::Senders {

/**
 * @brief Sends a fly targeting packet to the server to update projectile aiming.
 * 
 * @param stream The network stream interface to dispatch the packet.
 * @param targetId The entity ID (VID) of the target.
 * @param x The global X coordinate pixel position.
 * @param y The global Y coordinate pixel position.
 * @return EterBase::PacketResult<void> Returns success or a PacketError enum value on failure.
 */
EterBase::PacketResult<void> SendFlyTargetingPacket(
    CPythonNetworkStream& stream, 
    EterBase::EntityId targetId, 
    int32_t x, 
    int32_t y
);

} // namespace UserInterface::Network::Senders
