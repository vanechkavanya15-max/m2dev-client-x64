#include "StdAfx.h"
#include "../../PythonNetworkStream.h"
#include "../../Packet.h"
#include <cstdint>
#include <string_view>
#include <algorithm>

/**
 * @file QuestInputStringHandler.cpp
 * @brief Handler for sending quest string inputs to the server.
 */

/**
 * @brief Sends a quest input string packet to the server.
 * 
 * This method constructs a TPacketCGQuestInputString packet, safely copies the
 * input string into the packet buffer using std::string_view, and sends it 
 * over the network. It handles potential null pointers and truncates strings
 * that exceed the maximum length defined by QUEST_INPUT_STRING_MAX_NUM.
 * 
 * @param inputString The string input from the user/quest.
 * @return true if the packet was successfully sent; false otherwise.
 */
bool CPythonNetworkStream::SendQuestInputStringPacket(const char* inputString)
{
    TPacketCGQuestInputString packet{};
    packet.header = CG::QUEST_INPUT_STRING;
    packet.length = sizeof(packet);
    
    if (inputString != nullptr)
    {
        std::string_view stringView(inputString);
        size_t copyLength = std::min(stringView.length(), static_cast<size_t>(QUEST_INPUT_STRING_MAX_NUM));
        std::copy_n(stringView.begin(), copyLength, packet.szString);
    }

    if (!Send(sizeof(packet), &packet))
    {
        Tracen("SendQuestInputStringPacket Error");
        return false;
    }

    return true;
}
