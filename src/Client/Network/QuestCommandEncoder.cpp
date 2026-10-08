#include "StdAfx.h"
#include "QuestCommandEncoder.h"
#include "UserInterface/Packet.h"
#include <cstring>

namespace Client::Network {

    EterBase::PacketResult<std::vector<uint8_t>> QuestCommandEncoder::EncodeScriptAnswer(uint8_t answer) {
        TPacketCGScriptAnswer packet{};
        packet.header = CG::SCRIPT_ANSWER;
        packet.length = sizeof(packet);
        packet.answer = answer;

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));

        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> QuestCommandEncoder::EncodeOnClick(EterBase::EntityId targetId) {
        TPacketCGOnClick packet{};
        packet.header = CG::ON_CLICK;
        packet.length = sizeof(packet);
        packet.vid = targetId.value();

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));

        return buffer;
    }

} // namespace Client::Network
