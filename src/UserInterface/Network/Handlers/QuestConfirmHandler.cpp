#include "StdAfx.h"
#include "../../PythonNetworkStream.h"
#include "../../Packet.h"

#include <cstdint>
#include <span>

/**
 * @brief Class responsible for handling Quest Confirm network operations.
 * 
 * This handler is decoupled from the GUI and encapsulates the logic for sending
 * the user's answer (accept or reject) for a quest dialog directly to the server.
 */
class QuestConfirmHandler
{
public:
    /**
     * @brief Sends a quest confirmation packet to the server.
     * 
     * Constructs the network packet using standard C++20 standard types and span
     * representations, adhering to the zero-conflict rule and decoupling from UI components.
     * 
     * @param networkStream Pointer to the active PythonNetworkStream instance.
     * @param answer The user's selected choice (e.g., yes/no).
     * @param requestPid The PID of the entity requesting the quest confirmation.
     * @return true If the packet was successfully queued for sending.
     * @return false If the network stream is null or sending failed.
     */
    static bool SendQuestConfirmPacket(CPythonNetworkStream* networkStream, uint8_t answer, uint32_t requestPid)
    {
        if (!networkStream)
        {
            return false;
        }

        TPacketCGQuestConfirm packet{};
        packet.header = CG::QUEST_CONFIRM;
        packet.length = static_cast<uint16_t>(sizeof(packet));
        packet.answer = answer;
        packet.requestPID = requestPid;

        // Modern C++20 buffer view via std::span
        std::span<const uint8_t> packetSpan{ reinterpret_cast<const uint8_t*>(&packet), sizeof(packet) };

        if (!networkStream->Send(static_cast<int>(packetSpan.size()), packetSpan.data()))
        {
            Tracen("SendQuestConfirmPacket Error");
            return false;
        }

        Tracenf(" SendQuestConfirmPacket : %d, %d", answer, requestPid);
        return true;
    }
};
