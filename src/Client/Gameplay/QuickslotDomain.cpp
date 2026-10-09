#include "QuickslotDomain.h"
#include <algorithm>

namespace Client::Gameplay {

QuickslotDomain::QuickslotDomain()
{
    Clear();
}

void QuickslotDomain::Clear() noexcept
{
    std::unique_lock lock(m_mutex);
    m_slots.fill(QuickslotItem{0, 0});
    m_pageIndex = 0;
}

int32_t QuickslotDomain::GetPage() const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_pageIndex;
}

void QuickslotDomain::SetPage(int32_t pageIndex) noexcept
{
    std::unique_lock lock(m_mutex);
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

SlotIndex QuickslotDomain::LocalToGlobalIndexUnlocked(SlotIndex localSlotIndex) const noexcept
{
    return SlotIndex(static_cast<uint16_t>(static_cast<uint32_t>(m_pageIndex) * QUICKSLOT_MAX_COUNT_PER_LINE + localSlotIndex.get()));
}

SlotIndex QuickslotDomain::LocalToGlobalIndex(SlotIndex localSlotIndex) const noexcept
{
    std::shared_lock lock(m_mutex);
    return LocalToGlobalIndexUnlocked(localSlotIndex);
}

std::expected<QuickslotItem, EterBase::InventoryError> QuickslotDomain::GetSlotUnlocked(SlotIndex globalSlotIndex) const
{
    if (globalSlotIndex.get() >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    return m_slots[globalSlotIndex.get()];
}

std::expected<QuickslotItem, EterBase::InventoryError> QuickslotDomain::GetSlot(SlotIndex globalSlotIndex) const
{
    std::shared_lock lock(m_mutex);
    return GetSlotUnlocked(globalSlotIndex);
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::SetSlotUnlocked(SlotIndex globalSlotIndex, const QuickslotItem& item)
{
    if (globalSlotIndex.get() >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    m_slots[globalSlotIndex.get()] = item;
    return {};
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::SetSlot(SlotIndex globalSlotIndex, const QuickslotItem& item)
{
    std::unique_lock lock(m_mutex);
    return SetSlotUnlocked(globalSlotIndex, item);
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::ClearSlot(SlotIndex globalSlotIndex)
{
    std::unique_lock lock(m_mutex);
    if (globalSlotIndex.get() >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    m_slots[globalSlotIndex.get()] = QuickslotItem{0, 0};
    return {};
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::SwapSlots(SlotIndex slotA, SlotIndex slotB)
{
    std::unique_lock lock(m_mutex);
    if (slotA.get() >= QUICKSLOT_MAX_NUM || slotB.get() >= QUICKSLOT_MAX_NUM)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    std::swap(m_slots[slotA.get()], m_slots[slotB.get()]);
    return {};
}

std::expected<QuickslotItem, EterBase::InventoryError> QuickslotDomain::GetLocalSlot(SlotIndex localSlotIndex) const
{
    std::shared_lock lock(m_mutex);
    if (localSlotIndex.get() >= QUICKSLOT_MAX_COUNT_PER_LINE)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    SlotIndex globalIndex = LocalToGlobalIndexUnlocked(localSlotIndex);
    return GetSlotUnlocked(globalIndex);
}

std::expected<void, EterBase::InventoryError> QuickslotDomain::SetLocalSlot(SlotIndex localSlotIndex, const QuickslotItem& item)
{
    std::unique_lock lock(m_mutex);
    if (localSlotIndex.get() >= QUICKSLOT_MAX_COUNT_PER_LINE)
    {
        return std::unexpected(EterBase::InventoryError::SlotOutOfRange);
    }
    SlotIndex globalIndex = LocalToGlobalIndexUnlocked(localSlotIndex);
    return SetSlotUnlocked(globalIndex, item);
}

} // namespace Client::Gameplay
