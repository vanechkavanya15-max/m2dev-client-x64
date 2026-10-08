#include "MovementCommandEncoder.h"
#include "Protocol/Protocol.h"
#include <cmath>

namespace Client::Network {

EterBase::PacketResult<std::vector<uint8_t>> MovementCommandEncoder::Encode(const Core::MoveCommand& cmd) {
    TPacketCGMove packet{};
    
    packet.header = CG::MOVE;
    packet.length = sizeof(TPacketCGMove);
    
    constexpr uint8_t FUNC_MOVE = 1;
    packet.bFunc = FUNC_MOVE;
    packet.bArg = cmd.moveType;
    
    float rot = std::fmod(cmd.rotation, 360.0f);
    if (rot < 0.0f) {
        rot += 360.0f;
    }
    packet.bRot = static_cast<uint8_t>(rot / 5.0f);
    
    packet.lX = static_cast<int32_t>(cmd.destination.x * 100.0f);
    packet.lY = static_cast<int32_t>(cmd.destination.y * 100.0f);
    
    packet.dwTime = cmd.clientTimestamp;
    
    const uint8_t* rawData = reinterpret_cast<const uint8_t*>(&packet);
    return std::vector<uint8_t>(rawData, rawData + sizeof(TPacketCGMove));
}

} // namespace Client::Network
