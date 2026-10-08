#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <expected>

// Wymagane mocki do izolacji, zeby nie dolaczac StdAfx.h z d3d9.h w testach
#ifndef ETERBASE_STDAFX_H
#include "../../UserInterface/Packet.h"
#endif

#include "EterBase/Result.h"

namespace Client::Network
{
    using EterBase::PacketError;
    using EterBase::PacketResult;

    class PartyPacketCodec
    {
    public:
        static PacketResult<TPacketGCPartyInvite> DecodePartyInvite(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCPartyAdd> DecodePartyAdd(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCPartyUpdate> DecodePartyUpdate(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCPartyRemove> DecodePartyRemove(std::span<const uint8_t> buffer);
        static PacketResult<TPacketGCPartyParameter> DecodePartyParameter(std::span<const uint8_t> buffer);
        
        static std::vector<uint8_t> EncodePartyInviteAnswer(uint32_t leaderVid, uint8_t accept);
    };
}
