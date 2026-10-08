#include "EterBase/StdAfx.h"
#include "DungeonCommandEncoder.h"
#include "Protocol/Protocol.h"
#include <cstring>

namespace Client::Network {

EterBase::PacketResult<std::vector<uint8_t>> DungeonCommandEncoder::EncodeEnter(
    uint32_t dungeonId,
    bool isYmirFraming)
{
    if (isYmirFraming) {
        // 1B Header + 1B Action + 4B DungeonId
        std::vector<uint8_t> buffer(sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t));
        buffer[0] = static_cast<uint8_t>(CG::DUNGEON & 0xFF);
        buffer[1] = static_cast<uint8_t>(DungeonAction::Enter);
        std::memcpy(buffer.data() + 2, &dungeonId, sizeof(uint32_t));
        return buffer;
    } else {
        // 2B Header + 2B Length + 1B Action + 4B DungeonId
        uint16_t header = CG::DUNGEON;
        uint16_t length = sizeof(uint16_t) * 2 + sizeof(uint8_t) + sizeof(uint32_t);
        uint8_t action = static_cast<uint8_t>(DungeonAction::Enter);

        std::vector<uint8_t> buffer(length);
        std::memcpy(buffer.data(), &header, sizeof(uint16_t));
        std::memcpy(buffer.data() + 2, &length, sizeof(uint16_t));
        buffer[4] = action;
        std::memcpy(buffer.data() + 5, &dungeonId, sizeof(uint32_t));
        return buffer;
    }
}

EterBase::PacketResult<std::vector<uint8_t>> DungeonCommandEncoder::EncodeReady(bool isYmirFraming) {
    if (isYmirFraming) {
        std::vector<uint8_t> buffer(sizeof(uint8_t) + sizeof(uint8_t));
        buffer[0] = static_cast<uint8_t>(CG::DUNGEON & 0xFF);
        buffer[1] = static_cast<uint8_t>(DungeonAction::Ready);
        return buffer;
    } else {
        uint16_t header = CG::DUNGEON;
        uint16_t length = sizeof(uint16_t) * 2 + sizeof(uint8_t);
        uint8_t action = static_cast<uint8_t>(DungeonAction::Ready);

        std::vector<uint8_t> buffer(length);
        std::memcpy(buffer.data(), &header, sizeof(uint16_t));
        std::memcpy(buffer.data() + 2, &length, sizeof(uint16_t));
        buffer[4] = action;
        return buffer;
    }
}

EterBase::PacketResult<std::vector<uint8_t>> DungeonCommandEncoder::EncodeLeave(bool isYmirFraming) {
    if (isYmirFraming) {
        std::vector<uint8_t> buffer(sizeof(uint8_t) + sizeof(uint8_t));
        buffer[0] = static_cast<uint8_t>(CG::DUNGEON & 0xFF);
        buffer[1] = static_cast<uint8_t>(DungeonAction::Leave);
        return buffer;
    } else {
        uint16_t header = CG::DUNGEON;
        uint16_t length = sizeof(uint16_t) * 2 + sizeof(uint8_t);
        uint8_t action = static_cast<uint8_t>(DungeonAction::Leave);

        std::vector<uint8_t> buffer(length);
        std::memcpy(buffer.data(), &header, sizeof(uint16_t));
        std::memcpy(buffer.data() + 2, &length, sizeof(uint16_t));
        buffer[4] = action;
        return buffer;
    }
}

} // namespace Client::Network
