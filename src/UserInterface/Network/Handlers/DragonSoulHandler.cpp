#include "StdAfx.h"
#include "DragonSoulHandler.h"

#include <cstring>
#include <algorithm>

bool DragonSoulHandler::HandleReceiveRefinePacket(std::span<const uint8_t> packet_data)
{
    if (packet_data.size() < sizeof(TPacketGCDragonSoulRefine))
    {
        return false;
    }

    const auto* packet = reinterpret_cast<const TPacketGCDragonSoulRefine*>(packet_data.data());

    if (packet->header != GC::DRAGON_SOUL_REFINE)
    {
        return false;
    }

    switch (packet->bSubType)
    {
    case DragonSoulSub::OPEN:
        m_isWindowOpen = true;
        ClearAllRefineSlots();
        break;

    case DragonSoulSub::CLOSE:
        m_isWindowOpen = false;
        ClearAllRefineSlots();
        break;

    case DragonSoulSub::REFINE_FAIL:
    case DragonSoulSub::REFINE_FAIL_MAX_REFINE:
    case DragonSoulSub::REFINE_FAIL_INVALID_MATERIAL:
    case DragonSoulSub::REFINE_FAIL_NOT_ENOUGH_MONEY:
    case DragonSoulSub::REFINE_FAIL_NOT_ENOUGH_MATERIAL:
    case DragonSoulSub::REFINE_FAIL_TOO_MUCH_MATERIAL:
        // GUI should typically handle displaying the message, but our state remains unchanged 
        // regarding slot placement (or might require clearing based on server logic). 
        // For C++ state abstraction, we simply acknowledge the fail without modifying slots.
        break;

    case DragonSoulSub::REFINE_SUCCEED:
        // On success, the items might be consumed. 
        // Clear slots to reflect that they are no longer in the refine window.
        ClearAllRefineSlots();
        break;

    default:
        // Unhandled or invalid subtype
        return false;
    }

    return true;
}

bool DragonSoulHandler::AddItemToRefineSlot(uint32_t slot_index, const TItemPos& item_pos)
{
    if (slot_index >= DS_REFINE_WINDOW_MAX_NUM)
    {
        return false;
    }

    m_refineSlots[slot_index] = item_pos;
    return true;
}

void DragonSoulHandler::ClearRefineSlot(uint32_t slot_index)
{
    if (slot_index < DS_REFINE_WINDOW_MAX_NUM)
    {
        m_refineSlots[slot_index].reset();
    }
}

void DragonSoulHandler::ClearAllRefineSlots()
{
    for (auto& slot : m_refineSlots)
    {
        slot.reset();
    }
}

std::vector<uint8_t> DragonSoulHandler::BuildSendRefinePacket(uint8_t refine_type) const
{
    TPacketCGDragonSoulRefine packet{};
    packet.header = CG::DRAGON_SOUL_REFINE;
    packet.length = sizeof(packet);
    packet.bSubType = refine_type;

    for (size_t i = 0; i < DS_REFINE_WINDOW_MAX_NUM; ++i)
    {
        if (m_refineSlots[i].has_value())
        {
            packet.ItemGrid[i] = m_refineSlots[i].value();
        }
        else
        {
            // Empty slot, represented by an invalid cell according to TItemPos default constructor
            packet.ItemGrid[i] = TItemPos(INVENTORY, WORD_MAX);
        }
    }

    std::vector<uint8_t> data(sizeof(packet));
    std::memcpy(data.data(), &packet, sizeof(packet));

    return data;
}

bool DragonSoulHandler::IsWindowOpen() const
{
    return m_isWindowOpen;
}
