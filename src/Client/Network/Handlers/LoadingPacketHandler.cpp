#include "EterBase/StdAfx.h"
#include "LoadingPacketHandler.h"
#include "../../../UserInterface/Packet.h"
#include <cstring>

namespace Client::Network::Handlers {

Client::Core::Result<void, Client::Core::PacketError> LoadingPacketHandler::HandleMainCharacter(
    std::span<const uint8_t> payload, 
    Client::Core::WorldContext& context)
{
    if (payload.size() < sizeof(TPacketGCMainCharacter)) {
        return std::unexpected(Client::Core::PacketError::BufferUnderflow);
    }

    TPacketGCMainCharacter packet;
    std::memcpy(&packet, payload.data(), sizeof(TPacketGCMainCharacter));

    context.localPlayerVid = Client::Core::EntityVid(packet.dwVID);
    context.localPlayerCoords = { 
        static_cast<float>(packet.lX), 
        static_cast<float>(packet.lY), 
        static_cast<float>(packet.lZ) 
    };

    return {};
}

} // namespace Client::Network::Handlers
