#include "../../StdAfx.h"
#include "../../PythonNetworkStream.h"
#include "../../PythonSafeBox.h"
#include "../../Packet.h"

/**
 * @brief Handles the reception of the safe box size update packet.
 * 
 * This handler is responsible for processing the SAFEBOX_SIZE network packet.
 * It reads the packet size and updates the safe box capacity state in CPythonSafeBox.
 * Complies with C++20 standard, strict single responsibility, and decoupled from GUI calls.
 * 
 * @return true if the packet was successfully received and processed, false otherwise.
 */
bool CPythonNetworkStream::RecvSafeBoxSizePacket()
{
    TPacketGCSafeboxSize safeBoxSizePacket;
    if (!Recv(sizeof(safeBoxSizePacket), &safeBoxSizePacket))
    {
        return false;
    }

    // Update the internal safe box data structure
    CPythonSafeBox::Instance().OpenSafeBox(safeBoxSizePacket.bSize);
    PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OpenSafeboxWindow", Py_BuildValue("(i)", safeBoxSizePacket.bSize));

    return true;
}
