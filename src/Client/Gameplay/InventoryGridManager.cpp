#include "InventoryGridManager.h"

namespace Client::Gameplay {

InventoryGridManager::InventoryGridManager(uint16_t width, uint16_t height, uint16_t pages)
    : m_width(width)
    , m_height(height)
    , m_pages(pages)
    , m_pageSize(width * height)
    , m_totalSize(width * height * pages)
{
    m_grid.resize(m_totalSize, false);
}

Client::Core::Result<void, Client::Core::CommandError> InventoryGridManager::CanPlaceItem(EterBase::ItemSlot slot, uint8_t itemHeight) const {
    uint16_t startIdx = slot.get();
    
    if (startIdx >= m_totalSize) {
        return std::unexpected(Client::Core::CommandError::OutOfRange);
    }
    
    uint16_t startPage = startIdx / m_pageSize;
    
    for (uint8_t i = 0; i < itemHeight; ++i) {
        uint16_t currentIdx = startIdx + i * m_width;
        
        if (currentIdx >= m_totalSize) {
            return std::unexpected(Client::Core::CommandError::OutOfRange);
        }
        
        uint16_t currentPage = currentIdx / m_pageSize;
        if (currentPage != startPage) {
            return std::unexpected(Client::Core::CommandError::OutOfRange);
        }
        
        if (m_grid[currentIdx]) {
            return std::unexpected(Client::Core::CommandError::InventoryFull);
            
        }
    }
    
    return {};
}

Client::Core::Result<EterBase::ItemSlot, Client::Core::CommandError> InventoryGridManager::FindEmptySlot(uint8_t itemHeight) const {
    for (uint16_t i = 0; i < m_totalSize; ++i) {
        if (CanPlaceItem(EterBase::ItemSlot(i), itemHeight)) {
            return EterBase::ItemSlot(i);
        }
    }
    return std::unexpected(Client::Core::CommandError::InventoryFull);
}

Client::Core::Result<void, Client::Core::CommandError> InventoryGridManager::OccupySlot(EterBase::ItemSlot slot, uint8_t itemHeight) {
    auto res = CanPlaceItem(slot, itemHeight);
    if (!res) {
        return res;
    }
    
    uint16_t startIdx = slot.get();
    for (uint8_t i = 0; i < itemHeight; ++i) {
        m_grid[startIdx + i * m_width] = true;
    }
    
    return {};
}

Client::Core::Result<void, Client::Core::CommandError> InventoryGridManager::FreeSlot(EterBase::ItemSlot slot, uint8_t itemHeight) {
    uint16_t startIdx = slot.get();
    
    if (startIdx >= m_totalSize) {
        return std::unexpected(Client::Core::CommandError::OutOfRange);
    }
    
    uint16_t startPage = startIdx / m_pageSize;
    
    for (uint8_t i = 0; i < itemHeight; ++i) {
        uint16_t currentIdx = startIdx + i * m_width;
        
        if (currentIdx >= m_totalSize) {
            return std::unexpected(Client::Core::CommandError::OutOfRange);
        }
        
        uint16_t currentPage = currentIdx / m_pageSize;
        if (currentPage != startPage) {
            return std::unexpected(Client::Core::CommandError::OutOfRange);
        }
        
        if (!m_grid[currentIdx]) {
             // Maybe SlotEmpty is fitting here.
             // Actually, freeing an already empty slot is an error.
             return std::unexpected(Client::Core::CommandError::SlotEmpty);
        }
    }
    
    for (uint8_t i = 0; i < itemHeight; ++i) {
        m_grid[startIdx + i * m_width] = false;
    }
    
    return {};
}

} // namespace Client::Gameplay
