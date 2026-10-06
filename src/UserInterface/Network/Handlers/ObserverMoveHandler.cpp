#include "../../StdAfx.h"
#include "ObserverMoveHandler.h"
#include "../../PythonNetworkStream.h"
#include "../../PythonMiniMap.h"

bool ObserverMoveHandler::HandleObserverMove(CPythonNetworkStream* networkStream, std::span<const uint8_t> packetData)
{
    if (!networkStream)
        return false;

    if (packetData.size() < sizeof(TPacketGCObserverMove))
        return false;

    const auto* packet = reinterpret_cast<const TPacketGCObserverMove*>(packetData.data());

    // Update the mini-map observer position in C++ memory
    // Coordinates are scaled by 100.0f as per standard implementation
    CPythonMiniMap::Instance().MoveObserver(
        packet->vid,
        packet->x * 100.0f,
        packet->y * 100.0f
    );

    return true;
}
