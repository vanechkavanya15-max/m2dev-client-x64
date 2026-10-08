#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <string>
#include <expected>

#include "../../UserInterface/Packet.h"

namespace EterBase {
    enum class PacketError {
        BufferUnderflow,
        InvalidHeader
    };

    template <typename T>
    using PacketResult = std::expected<T, PacketError>;
}

namespace Client::Network {

    class QuestPacketCodec {
    public:
        static EterBase::PacketResult<TPacketGCQuestInfo> DecodeQuestInfo(std::span<const uint8_t> buffer);
        static EterBase::PacketResult<TPacketGCQuestConfirm> DecodeQuestConfirm(std::span<const uint8_t> buffer);
        static EterBase::PacketResult<std::string> DecodeScript(std::span<const uint8_t> buffer);

        static std::vector<uint8_t> EncodeScriptAnswer(int32_t answerIndex);
        static std::vector<uint8_t> EncodeQuestConfirmAnswer(uint8_t answer, uint32_t pid);
    };

}
