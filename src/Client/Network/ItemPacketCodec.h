#pragma once

#include <vector>
#include <expected>
#include <cstdint>
#include <string>

// To avoid monolithic dependencies, we mock the specific packet structs locally
// or rely on UserInterface/Packet.h if available. Since Zero-Conflict specifies
// we only add to our directories, we should rely on existing structs from UserInterface/Packet.h
// Wait, to safely include UserInterface/Packet.h we might need some macros. 
// For pure codec, we should use our own structs or UserInterface ones?
// "TPacketCGItemUse, TPacketCGItemMove, TPacketCGItemDrop, TPacketGCItemSet, TPacketGCItemDel, TPacketGCItemGroundAdd"
// The prompt tells us "Kodowanie i dekodowanie operacji na przedmiotach: TPacket..."
// Let's include the headers where these packets are defined.

#include "EterBase/StrongTypes.h"
#include "UserInterface/Packet.h"

namespace Client::Network {

/**
 * @brief Codec for Item Packets.
 */
class ItemPacketCodec {
public:
    // --- TPacketCGItemUse ---
    static std::expected<std::vector<uint8_t>, std::string> EncodeItemUse(const TPacketCGItemUse& packet);
    static std::expected<TPacketCGItemUse, std::string> DecodeItemUse(const std::vector<uint8_t>& buffer);

    // --- TPacketCGItemMove ---
    static std::expected<std::vector<uint8_t>, std::string> EncodeItemMove(const TPacketCGItemMove& packet);
    static std::expected<TPacketCGItemMove, std::string> DecodeItemMove(const std::vector<uint8_t>& buffer);

    // --- TPacketCGItemDrop ---
    static std::expected<std::vector<uint8_t>, std::string> EncodeItemDrop(const TPacketCGItemDrop& packet);
    static std::expected<TPacketCGItemDrop, std::string> DecodeItemDrop(const std::vector<uint8_t>& buffer);

    // --- TPacketGCItemSet ---
    static std::expected<std::vector<uint8_t>, std::string> EncodeItemSet(const TPacketGCItemSet& packet);
    static std::expected<TPacketGCItemSet, std::string> DecodeItemSet(const std::vector<uint8_t>& buffer);

    // --- TPacketGCItemDel ---
    static std::expected<std::vector<uint8_t>, std::string> EncodeItemDel(const TPacketGCItemDel& packet);
    static std::expected<TPacketGCItemDel, std::string> DecodeItemDel(const std::vector<uint8_t>& buffer);

    // --- TPacketGCItemGroundAdd ---
    static std::expected<std::vector<uint8_t>, std::string> EncodeItemGroundAdd(const TPacketGCItemGroundAdd& packet);
    static std::expected<TPacketGCItemGroundAdd, std::string> DecodeItemGroundAdd(const std::vector<uint8_t>& buffer);
};

} // namespace Client::Network
