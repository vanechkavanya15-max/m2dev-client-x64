#include "RefineCommandEncoder.h"
#include <cstring>

namespace Client::Network {

[[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> RefineCommandEncoder::EncodeStandardRefine(EterBase::ItemSlot slot, uint8_t refineType)
{
    // TPacketCGRefine requires an 8-bit position.
    if (slot.value() > 255) {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    TPacketCGRefine packet{};
    packet.header = CG::REFINE;
    packet.length = sizeof(TPacketCGRefine);
    packet.pos = static_cast<uint8_t>(slot.value());
    packet.type = refineType;

    std::vector<uint8_t> buffer(sizeof(TPacketCGRefine));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketCGRefine));

    return buffer;
}

[[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> RefineCommandEncoder::EncodeDragonSoulRefine(uint8_t subType, std::span<const TItemPos> gridItems)
{
    if (gridItems.size() > DS_REFINE_WINDOW_MAX_NUM) {
        return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
    }

    TPacketCGDragonSoulRefine packet;
    packet.header = CG::DRAGON_SOUL_REFINE;
    packet.length = sizeof(TPacketCGDragonSoulRefine);
    packet.bSubType = subType;

    for (size_t i = 0; i < DS_REFINE_WINDOW_MAX_NUM; ++i) {
        if (i < gridItems.size()) {
            packet.ItemGrid[i] = gridItems[i];
        } else {
            // Unfilled slots should be represented as an empty cell.
            // TItemPos default constructor handles invalidation, but explicitly setting it:
            packet.ItemGrid[i] = TItemPos(INVENTORY, WORD_MAX);
        }
    }

    std::vector<uint8_t> buffer(sizeof(TPacketCGDragonSoulRefine));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketCGDragonSoulRefine));

    return buffer;
}

} // namespace Client::Network
