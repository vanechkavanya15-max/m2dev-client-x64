#include "QuestPacketCodec.h"
#include <cstring>

namespace Client::Network {

    EterBase::PacketResult<TPacketGCQuestInfo> QuestPacketCodec::DecodeQuestInfo(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCQuestInfo)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCQuestInfo packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCQuestInfo));
        return packet;
    }

    EterBase::PacketResult<TPacketGCQuestConfirm> QuestPacketCodec::DecodeQuestConfirm(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCQuestConfirm)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCQuestConfirm packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCQuestConfirm));
        return packet;
    }

    EterBase::PacketResult<std::string> QuestPacketCodec::DecodeScript(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TDynamicSizePacketHeader)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        TDynamicSizePacketHeader header;
        std::memcpy(&header, buffer.data(), sizeof(TDynamicSizePacketHeader));

        if (buffer.size() < header.length) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        size_t scriptLength = header.length - sizeof(TDynamicSizePacketHeader);
        std::string script;
        script.resize(scriptLength);
        std::memcpy(script.data(), buffer.data() + sizeof(TDynamicSizePacketHeader), scriptLength);
        
        // Remove trailing null bytes if any
        while (!script.empty() && script.back() == '\0') {
            script.pop_back();
        }

        return script;
    }

    std::vector<uint8_t> QuestPacketCodec::EncodeScriptAnswer(int32_t answerIndex) {
        std::vector<uint8_t> buffer(sizeof(TPacketCGScriptAnswer));
        TPacketCGScriptAnswer packet = {};
        packet.header = CG::SCRIPT_ANSWER; // Needs to be CG::SCRIPT_ANSWER in real context, it will compile since we mock it
        packet.length = sizeof(TPacketCGScriptAnswer);
        packet.answer = static_cast<uint8_t>(answerIndex);
        
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGScriptAnswer));
        return buffer;
    }

    std::vector<uint8_t> QuestPacketCodec::EncodeQuestConfirmAnswer(uint8_t answer, uint32_t pid) {
        std::vector<uint8_t> buffer(sizeof(TPacketCGQuestConfirm));
        TPacketCGQuestConfirm packet = {};
        packet.header = CG::QUEST_CONFIRM; 
        packet.length = sizeof(TPacketCGQuestConfirm);
        packet.answer = answer;
        packet.requestPID = pid;

        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGQuestConfirm));
        return buffer;
    }

}
