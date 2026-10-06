#include "StdAfx.h"
#include "SendMoveHandler.h"
#include "../../PythonBackground.h"
#include <cmath>
#include <cstring>

namespace Network
{
    /**
     * @brief Formats and sends a movement packet to the server using the Beavium protocol.
     * 
     * @param targetPosition The pixel position of the movement target in local space.
     * @param rotation The facing rotation angle in degrees.
     * @param func The function/state of movement (e.g. FUNC_MOVE, FUNC_WAIT).
     * @param arg Additional argument for the state.
     * @param sendCallback A callback function taking a span of bytes to send over the network.
     * @return true if the packet was successfully sent, false otherwise.
     */
    bool SendMovePacket(const TPixelPosition& targetPosition, float rotation, uint8_t func, uint16_t arg, const std::function<bool(std::span<const uint8_t>)>& sendCallback)
    {
        if (rotation < 0.0f)
            rotation = 360.0f + std::fmod(rotation, 360.0f);
        else if (rotation > 360.0f)
            rotation = std::fmod(rotation, 360.0f);

        int32_t x = static_cast<int32_t>(targetPosition.x);
        int32_t y = static_cast<int32_t>(targetPosition.y);
        
        CPythonBackground::Instance().LocalPositionToGlobalPosition(x, y);

#pragma pack(push, 1)
        Beavium::TPacketCGMoveBeavium movePacket;
#pragma pack(pop)
        
        std::memset(&movePacket, 0, sizeof(movePacket));
        
        movePacket.header = Beavium::CG::MOVE;
        movePacket.bFunc = func;
        movePacket.wArg = arg;
        movePacket.dwRot = Beavium::EncodeRotationMicrodegrees(rotation);
        movePacket.lX = x;
        movePacket.lY = y;
        movePacket.dwTime = ELTimer_GetServerMSec();
        movePacket.dwExtra = 0;

        std::span<const uint8_t> packetSpan(reinterpret_cast<const uint8_t*>(&movePacket), sizeof(movePacket));

        if (!sendCallback(packetSpan))
        {
            Tracenf("[BEAVIUM] SendMovePacket error (x=%d, y=%d, rot=%f)", x, y, rotation);
            return false;
        }

        return true;
    }
}
