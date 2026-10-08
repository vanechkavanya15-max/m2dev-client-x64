#include "QuickslotDomain.h"
#include <algorithm>

namespace Client::Gameplay {

QuickslotDomain::QuickslotDomain()
{
    Clear();
}

void QuickslotDomain::Clear() noexcept
{
    m_slots.fill(QuickslotItem{0, 0});
    m_pageIndex = 0;
}

void QuickslotDomain::SetPage(int32_t pageIndex) noexcept
{
    if (pageIndex < 0)
    {
        m_pageIndex = static_cast<int32_t>(QUICKSLOT_MAX_LINE) + pageIndex;
        if (m_pageIndex < 0)
        {
            m_pageIndex = 0;
        }
    }
    else if (pageIndex >= static_cast<int32_t>(QUICKSLOT_MAX_LINE))
    {
        m_pageIndex = pageIndex % static_cast<int32_t>(QUICKSLOT_MAX_LINE);
    }
    else
    {
        m_pageIndex = pageIndex;
    }
}

SlotIndex QuickslotDomain::LocalToGlobalIndex(SlotIndex localSlotIndex) const noexcept
{
    return SlotIndex(static_cast<uint16_t>(static_cast<uint32_t>(m_pageIndex) * QUICKSLOT_MAX_COUNT_PER_LINE + localSlotIndex.get()));
}

std::expected<QuickslotItem, EterBase::InventoryError> QuickslotDomain::GetSlot(SlotIndex globalSlotIndex) const
{
    if (globalSlotIndex.get() >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    return m_slots[globalSlotIndex.get()];
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::SetSlot(SlotIndex globalSlotIndex, const QuickslotItem& item)
{
    if (globalSlotIndex.get() >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    m_slots[globalSlotIndex.get()] = item;
    return {};
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::ClearSlot(SlotIndex globalSlotIndex)
{
    if (globalSlotIndex.get() >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    m_slots[globalSlotIndex.get()] = QuickslotItem{0, 0};
    return {};
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::SwapSlots(SlotIndex slotA, SlotIndex slotB)
{
    if (slotA.get() >= QUICKSLOT_MAX_NUM || slotB.get() >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    std::swap(m_slots[slotA.get()], m_slots[slotB.get()]);
    return {};
}

std::expected<QuickslotItem, EterBase::InventoryError> QuickslotDomain::GetLocalSlot(SlotIndex localSlotIndex) const
{
    if (localSlotIndex.get() >= QUICKSLOT_MAX_COUNT_PER_LINE)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    SlotIndex globalIndex = LocalToGlobalIndex(localSlotIndex);
    return GetSlot(globalIndex);
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::SetLocalSlot(SlotIndex localSlotIndex, const QuickslotItem& item)
{
    if (localSlotIndex.get() >= QUICKSLOT_MAX_COUNT_PER_LINE)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    SlotIndex globalIndex = LocalToGlobalIndex(localSlotIndex);
    return SetSlot(globalIndex, item);
}

} // namespace Client::Gameplay
