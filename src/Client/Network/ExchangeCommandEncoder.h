#pragma once

#include "EterBase/PacketResult.h"
#include "Protocol/ProtocolTypes.h"
#include <vector>
#include <cstdint>

namespace Client::Network
{
    /**
     * @brief Encodes exchange commands (START, ITEM_ADD, ITEM_DEL, ELK_ADD, ACCEPT, CANCEL) into CG_EXCHANGE packet bytes.
     */
    class ExchangeCommandEncoder
    {
    public:
        ExchangeCommandEncoder() = default;
        ~ExchangeCommandEncoder() = default;

        ExchangeCommandEncoder(const ExchangeCommandEncoder&) = delete;
        ExchangeCommandEncoder& operator=(const ExchangeCommandEncoder&) = delete;

        EterBase::PacketResult<std::vector<uint8_t>> EncodeStart(uint32_t targetVid) const;
        EterBase::PacketResult<std::vector<uint8_t>> EncodeItemAdd(TItemPos itemPos, uint8_t displayPos) const;
        EterBase::PacketResult<std::vector<uint8_t>> EncodeItemDel(uint8_t pos) const;
        EterBase::PacketResult<std::vector<uint8_t>> EncodeElkAdd(uint32_t amount) const;
        EterBase::PacketResult<std::vector<uint8_t>> EncodeAccept() const;
        EterBase::PacketResult<std::vector<uint8_t>> EncodeCancel() const;
    };
}
