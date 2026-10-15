#pragma once

#include "StdAfx.h"
#include <cstdint>
#include <span>
#include <vector>

#include "../../EterBase/Result.h"
#include "Protocol/Protocol.h"

namespace Network::CombatPacketCodec
{
    /**
     * @brief Encodes a CG::ATTACK packet into a byte buffer.
     */
    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeAttack(const TPacketCGAttack& packet);

    /**
     * @brief Decodes a CG::ATTACK packet from a byte buffer.
     */
    [[nodiscard]] EterBase::PacketResult<TPacketCGAttack> DecodeAttack(std::span<const uint8_t> buffer);

    /**
     * @brief Encodes a CG::SHOOT packet into a byte buffer.
     */
    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeShoot(const TPacketCGShoot& packet);

    /**
     * @brief Decodes a CG::SHOOT packet from a byte buffer.
     */
    [[nodiscard]] EterBase::PacketResult<TPacketCGShoot> DecodeShoot(std::span<const uint8_t> buffer);

    /**
     * @brief Encodes a GC::DAMAGE_INFO packet into a byte buffer.
     */
    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeDamageInfo(const TPacketGCDamageInfo& packet);

    /**
     * @brief Decodes a GC::DAMAGE_INFO packet from a byte buffer.
     * Safely validates the damage flag.
     */
    [[nodiscard]] EterBase::PacketResult<TPacketGCDamageInfo> DecodeDamageInfo(std::span<const uint8_t> buffer);

    /**
     * @brief Encodes a GC::DEAD packet into a byte buffer.
     */
    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> EncodeDead(const TPacketGCDead& packet);

    /**
     * @brief Decodes a GC::DEAD packet from a byte buffer.
     */
    [[nodiscard]] EterBase::PacketResult<TPacketGCDead> DecodeDead(std::span<const uint8_t> buffer);
}
