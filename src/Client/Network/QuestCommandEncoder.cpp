#include "QuestCommandEncoder.h"

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

namespace CG {
    constexpr uint16_t SCRIPT_ANSWER      = 0x0901;
    constexpr uint16_t ON_CLICK           = 0x0A02;
}

#pragma pack(push, 1)
typedef struct command_script_answer
{
    uint16_t	header;
    uint16_t	length;
	uint8_t		answer;
} TPacketCGScriptAnswer;

typedef struct command_on_click
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		vid;
} TPacketCGOnClick;
#pragma pack(pop)
