#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <string>
#include <expected>

#include "EterBase/Result.h"
#include "Protocol/Protocol.h"

namespace Client::Network {

    class QuestPacketCodec {
    public:
        static bool DecodeQuestInfo(const TPacketGCQuestInfo& pack, uint16_t& outIndex, uint8_t& outFlag)
        {
            outIndex = pack.index;
            outFlag = pack.flag;
            return true;
        }

        static bool DecodeQuestConfirm(const TPacketGCQuestConfirm& pack, std::string& outMsg, int32_t& outTimeout, uint32_t& outRequestPID)
        {
            outMsg = pack.msg;
            outTimeout = pack.timeout;
            outRequestPID = pack.requestPID;
            return true;
        }

        static EterBase::PacketResult<TPacketGCQuestInfo> DecodeQuestInfo(std::span<const uint8_t> buffer);
        static EterBase::PacketResult<TPacketGCQuestConfirm> DecodeQuestConfirm(std::span<const uint8_t> buffer);
        static EterBase::PacketResult<std::string> DecodeScript(std::span<const uint8_t> buffer);

        static std::vector<uint8_t> EncodeScriptAnswer(int32_t answerIndex);
        static std::vector<uint8_t> EncodeQuestConfirmAnswer(uint8_t answer, uint32_t pid);
    };

}
