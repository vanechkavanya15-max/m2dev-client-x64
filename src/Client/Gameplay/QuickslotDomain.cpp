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

uint32_t QuickslotDomain::LocalToGlobalIndex(uint32_t localSlotIndex) const noexcept
{
    return static_cast<uint32_t>(m_pageIndex) * QUICKSLOT_MAX_COUNT_PER_LINE + localSlotIndex;
}

std::expected<QuickslotItem, EterBase::InventoryError> QuickslotDomain::GetSlot(uint32_t globalSlotIndex) const
{
    if (globalSlotIndex >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    return m_slots[globalSlotIndex];
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::SetSlot(uint32_t globalSlotIndex, const QuickslotItem& item)
{
    if (globalSlotIndex >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    m_slots[globalSlotIndex] = item;
    return {};
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::ClearSlot(uint32_t globalSlotIndex)
{
    if (globalSlotIndex >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    m_slots[globalSlotIndex] = QuickslotItem{0, 0};
    return {};
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::SwapSlots(uint32_t slotA, uint32_t slotB)
{
    if (slotA >= QUICKSLOT_MAX_NUM || slotB >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    std::swap(m_slots[slotA], m_slots[slotB]);
    return {};
}

std::expected<QuickslotItem, EterBase::InventoryError> QuickslotDomain::GetLocalSlot(uint32_t localSlotIndex) const
{
    if (localSlotIndex >= QUICKSLOT_MAX_COUNT_PER_LINE)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    uint32_t globalIndex = LocalToGlobalIndex(localSlotIndex);
    return GetSlot(globalIndex);
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::SetLocalSlot(uint32_t localSlotIndex, const QuickslotItem& item)
{
    if (localSlotIndex >= QUICKSLOT_MAX_COUNT_PER_LINE)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    uint32_t globalIndex = LocalToGlobalIndex(localSlotIndex);
    return SetSlot(globalIndex, item);
}

} // namespace Client::Gameplay
