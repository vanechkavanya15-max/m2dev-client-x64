#include "StdAfx.h"
#include "SendMoveHandler.h"
#include "../../PythonBackground.h"
#include "Client/Network/Protocol/ProtocolDriverRegistry.h"
#include "Client/Network/Domain/MovementCommands.h"
#include <cmath>

namespace Network
{
    /**
     * @brief Formats and sends a movement packet to the server using the active protocol driver.
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

        auto* pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
        if (!pDriver)
        {
            Network::Protocol::ProtocolDriverRegistry::Instance().InitializeDefaults();
            pDriver = Network::Protocol::ProtocolDriverRegistry::Instance().GetActiveDriver();
        }

        if (!pDriver)
            return false;

        Network::Domain::MoveCommand cmd{
            .vid = 0,
            .x = x,
            .y = y,
            .rotationDegrees = rotation,
            .time = ELTimer_GetServerMSec(),
            .func = func,
            .arg = static_cast<uint8_t>(arg)
        };

        auto encodedResult = pDriver->EncodeMove(cmd);
        if (!encodedResult.has_value())
        {
            Tracenf("[PROTOCOL] SendMovePacket encoding error (x=%d, y=%d, rot=%f)", x, y, rotation);
            return false;
        }

        const auto& buffer = encodedResult.value();
        std::span<const uint8_t> packetSpan(buffer.data(), buffer.size());

        if (!sendCallback(packetSpan))
        {
            Tracenf("[PROTOCOL] SendMovePacket send callback failed (x=%d, y=%d, rot=%f)", x, y, rotation);
            return false;
        }

        return true;
    }
}
