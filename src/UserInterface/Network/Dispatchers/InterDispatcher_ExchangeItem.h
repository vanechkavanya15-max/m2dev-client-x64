#pragma once
#include <cstdint>
#include <span>
#include "../../../EterBase/Result.h"

namespace Network::Dispatchers {
    EterBase::PacketResult<void> DispatchExchangeItem(std::span<const uint8_t> buffer);
}
