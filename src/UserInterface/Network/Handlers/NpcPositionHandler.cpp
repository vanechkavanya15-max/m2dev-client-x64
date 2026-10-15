#include "StdAfx.h"
#include "../../PythonNetworkStream.h"
#include "../../PythonMiniMap.h"
#include "../../PythonNonPlayer.h"
#include "../../Packet.h"

#include <cstdint>
#include <cassert>
#include <string_view>
#include <span>

/**
 * @brief Handles incoming NPC position packets from the server and updates the minimap.
 * 
 * Reads the NPC position packet header, clears the current atlas mark info, and reads each
 * NPC position entry to register it on the minimap atlas. Uses modern C++ standard types
 * and avoids direct UI manipulation by only updating internal memory states.
 * 
 * @return true if the packet was successfully parsed, false otherwise.
 */
bool CPythonNetworkStream::RecvNPCList()
{
    TPacketGCNPCPosition packetNpcPosition;
    if (!Recv(sizeof(packetNpcPosition), &packetNpcPosition))
    {
        return false;
    }

    assert(static_cast<int32_t>(packetNpcPosition.length) - sizeof(packetNpcPosition) == 
           packetNpcPosition.count * sizeof(TNPCPosition) && "GC::NPC_POSITION size mismatch");

    CPythonMiniMap::Instance().ClearAtlasMarkInfo();

    for (uint16_t i = 0; i < packetNpcPosition.count; ++i)
    {
        TNPCPosition npcPosition;
        
        std::span<uint8_t> bufferSpan(reinterpret_cast<uint8_t*>(&npcPosition), sizeof(npcPosition));
        if (!Recv(bufferSpan.size_bytes(), bufferSpan.data()))
        {
            return false;
        }

        const char* pNpcName = nullptr;
        if (CPythonNonPlayer::Instance().GetName(npcPosition.dwVnum, &pNpcName) && pNpcName != nullptr)
        {
            std::string_view npcName(pNpcName);
            CPythonMiniMap::Instance().RegisterAtlasMark(npcPosition.bType, npcName.data(), npcPosition.x, npcPosition.y);
        }
        else
        {
            std::string_view defaultName(npcPosition.name);
            CPythonMiniMap::Instance().RegisterAtlasMark(npcPosition.bType, defaultName.data(), npcPosition.x, npcPosition.y);
        }
    }

    return true;
}
