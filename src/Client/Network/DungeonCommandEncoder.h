#pragma once

#include "EterBase/StdAfx.h"
#include <vector>
#include <cstdint>
#include "EterBase/Result.h"

namespace Client::Network {

enum class DungeonAction : uint8_t {
    Enter = 1,
    Ready = 2,
    Leave = 3
};

class DungeonCommandEncoder {
public:
    DungeonCommandEncoder() = default;
    ~DungeonCommandEncoder() = default;

    [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeEnter(
        uint32_t dungeonId,
        bool isYmirFraming);

    [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeReady(
        bool isYmirFraming);

    [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeLeave(
        bool isYmirFraming);
};

} // namespace Client::Network
