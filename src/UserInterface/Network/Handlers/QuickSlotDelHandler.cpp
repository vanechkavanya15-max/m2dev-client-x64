#include "StdAfx.h"
#include "QuickSlotDelHandler.h"
#include "../../AbstractPlayer.h"
#include "../../Packet.h"

/**
 * @brief Processes QuickSlotDel network payload, updating the local C++ state.
 * @param packet Network buffer containing the GC QuickSlotDel struct.
 * @return True if parsing and state update succeeds, false if the buffer is too small.
 */
bool HandleQuickSlotDelPacket(std::span<const uint8_t> packet)
{
    if (packet.size() < sizeof(TPacketGCQuickSlotDel))
        return false;

    const auto* data = reinterpret_cast<const TPacketGCQuickSlotDel*>(packet.data());

    IAbstractPlayer& player = IAbstractPlayer::GetSingleton();
    player.DeleteQuickSlot(data->pos);

    return true;
}
