#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../PythonNetworkStream.h"
#include "../../PythonPlayer.h"
#include "../../PythonApplication.h"
#include <cstdint>

/**
 * @brief Handler for adding buffs/affects to the player character.
 * 
 * This class is responsible for reading the AffectAdd packet from the
 * server and updating the local client's memory state accordingly.
 * It is completely decoupled from the GUI, following the C++20 modernization principles.
 */
class AffectAddHandler
{
public:
    /**
     * @brief Processes the TPacketGCAffectAdd packet from the network stream.
     * 
     * Reads the packet payload from the network stream, deserializes it,
     * and applies the affect to the current player instance's state.
     * 
     * @param stream The active Python network stream to read the packet from.
     * @return true if the packet was successfully processed, false otherwise.
     */
    static bool Process(CPythonNetworkStream& stream)
    {
        TPacketGCAffectAdd packet;
        
        if (!stream.Recv(sizeof(packet), &packet))
        {
            return false;
        }

        const TPacketAffectElement& element = packet.elem;

        // Apply energy duration specifically if the apply type matches POINT_ENERGY
        if (element.bPointIdxApplyOn == POINT_ENERGY)
        {
            time_t server_time = CPythonApplication::Instance().GetServerTimeStamp();
            CPythonPlayer::Instance().SetStatus(POINT_ENERGY_END_TIME, server_time + element.lDuration);
            // We assume a CPythonPlayer method or a way to update status without UI
        }

        // Apply general affect to player memory state
        CPythonPlayer::Instance().SetAffect(element.dwType);

        return true;
    }
};
