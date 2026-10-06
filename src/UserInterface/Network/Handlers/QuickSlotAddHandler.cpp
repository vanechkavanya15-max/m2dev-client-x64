#include "../../StdAfx.h"
#include "QuickSlotAddHandler.hpp"
#include "../../Packet.h"
#include "../../PythonNetworkStream.h"

namespace Network
{
namespace Handlers
{

bool QuickSlotAddHandler::SendQuickSlotAddPacket(uint8_t windowPosition, uint8_t type, uint8_t position)
{
    CPythonNetworkStream& stream = CPythonNetworkStream::Instance();

    TPacketCGQuickSlotAdd packet;
    packet.header = CG::QUICKSLOT_ADD;
    packet.length = sizeof(packet);
    packet.pos = windowPosition;
    packet.slot.Type = type;
    packet.slot.Position = position;

    if (!stream.Send(sizeof(packet), &packet))
    {
        return false;
    }

    return true;
}

} // namespace Handlers
} // namespace Network
