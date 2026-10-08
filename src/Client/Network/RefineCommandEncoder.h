#pragma once

#include <vector>
#include <span>
#include <cstdint>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/PacketResult.h"
#include "Protocol/Protocol.h"

namespace Client::Network {

/**
 * @class RefineCommandEncoder
 * @brief Encoder for refine-related network commands, adhering to the Zero-Conflict Architecture.
 */
class RefineCommandEncoder {
public:
    /**
     * @brief Encodes a standard refine or bless refine command.
     * @param slot The inventory slot of the item being refined.
     * @param refineType The type of refine (e.g., standard, scroll).
     * @return PacketResult containing the encoded byte buffer.
     */
    [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeStandardRefine(EterBase::ItemSlot slot, uint8_t refineType);

    /**
     * @brief Encodes a Dragon Soul refine command.
     * @param subType The refine sub-type (e.g., UPGRADE, IMPROVEMENT, REFINE).
     * @param gridItems A span representing the items placed in the Dragon Soul refine window grid.
     * @return PacketResult containing the encoded byte buffer.
     */
    [[nodiscard]] static EterBase::PacketResult<std::vector<uint8_t>> EncodeDragonSoulRefine(uint8_t subType, std::span<const TItemPos> gridItems);
};

} // namespace Client::Network
