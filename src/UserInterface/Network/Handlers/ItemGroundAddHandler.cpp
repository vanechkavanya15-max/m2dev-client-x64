#include "StdAfx.h"
#include "ItemGroundAddHandler.h"
#include "../../PythonItem.h"
#include "../../PythonNetworkStream.h"
#include "../../PythonBackground.h"
#include <cstring>

/**
 * @brief Processes the item ground add packet buffer.
 * 
 * @param buffer The binary span representing the network packet payload.
 * @return true if the buffer was valid and successfully parsed; false otherwise.
 */
bool ItemGroundAddHandler::Handle(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(ItemGroundAddPacket))
    {
        return false;
    }

    ItemGroundAddPacket packet;
    std::memcpy(&packet, buffer.data(), sizeof(ItemGroundAddPacket));

    int32_t x = packet.x;
    int32_t y = (packet.y > 10) ? (packet.y * 100) : packet.y;

    CPythonBackground::Instance().GlobalPositionToLocalPosition(x, y);


    TPixelPosition groundCoords(static_cast<float>(x), static_cast<float>(y), static_cast<float>(packet.z));
    CPythonItem::Instance().CreateItem(
        packet.id,
        packet.vnum,
        groundCoords
    );

    return true;
}
