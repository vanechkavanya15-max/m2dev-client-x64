#include "StdAfx.h"
#include "ExchangeCommandEncoder.h"
#include "UserInterface/Packet.h"

#include <cstring>

namespace Client::Network
{
    static EterBase::PacketResult<std::vector<uint8_t>> EncodeCommand(uint8_t subheader, uint32_t value1 = 0, uint8_t value2 = 0, TItemPos itemPos = TItemPos())
    {
        TPacketCGExchange packet;
        packet.header = CG::EXCHANGE;
        packet.length = sizeof(TPacketCGExchange);
        packet.subheader = subheader;
        packet.arg1 = value1;
        packet.arg2 = value2;
        packet.Pos = itemPos;

        std::vector<uint8_t> buffer(sizeof(TPacketCGExchange));
        std::memcpy(buffer.data(), &packet, sizeof(TPacketCGExchange));

        return buffer;
    }

    EterBase::PacketResult<std::vector<uint8_t>> ExchangeCommandEncoder::EncodeStart(uint32_t targetVid) const
    {
        return EncodeCommand(ExchangeSub::CG::START, targetVid);
    }

    EterBase::PacketResult<std::vector<uint8_t>> ExchangeCommandEncoder::EncodeItemAdd(TItemPos itemPos, uint8_t displayPos) const
    {
        return EncodeCommand(ExchangeSub::CG::ITEM_ADD, 0, displayPos, itemPos);
    }

    EterBase::PacketResult<std::vector<uint8_t>> ExchangeCommandEncoder::EncodeItemDel(uint8_t pos) const
    {
        return EncodeCommand(ExchangeSub::CG::ITEM_DEL, 0, pos);
    }

    EterBase::PacketResult<std::vector<uint8_t>> ExchangeCommandEncoder::EncodeElkAdd(uint32_t amount) const
    {
        return EncodeCommand(ExchangeSub::CG::ELK_ADD, amount);
    }

    EterBase::PacketResult<std::vector<uint8_t>> ExchangeCommandEncoder::EncodeAccept() const
    {
        return EncodeCommand(ExchangeSub::CG::ACCEPT);
    }

    EterBase::PacketResult<std::vector<uint8_t>> ExchangeCommandEncoder::EncodeCancel() const
    {
        return EncodeCommand(ExchangeSub::CG::CANCEL);
    }
}
