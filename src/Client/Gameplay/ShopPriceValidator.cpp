#include "../../StdAfx.h"
#include "ShopPriceValidator.h"
#include <limits>

namespace Client::Gameplay {

ShopPriceValidator::ShopPriceValidator(const Core::WorldContext& context) noexcept
    : m_context(context)
{
}

std::expected<int64_t, Core::CommandError> ShopPriceValidator::ValidatePurchase(
    CurrencyType currency,
    int64_t unitPrice,
    uint32_t count) const noexcept
{
    if (unitPrice < 0 || count == 0) {
        return std::unexpected(Core::CommandError::InvalidParameter);
    }

    if (unitPrice > 0 && count > 0 && unitPrice > std::numeric_limits<int64_t>::max() / static_cast<int64_t>(count)) {
        return std::unexpected(Core::CommandError::InvalidParameter);
    }
    
    int64_t totalCost = unitPrice * static_cast<int64_t>(count);

    switch (currency) {
        case CurrencyType::Gold:
            if (m_context.currentGold < totalCost) {
                return std::unexpected(Core::CommandError::InvalidParameter); 
            }
            return m_context.currentGold - totalCost;
            
        case CurrencyType::Cheque:
            if (m_context.currentCheque < totalCost) {
                return std::unexpected(Core::CommandError::InvalidParameter);
            }
            return m_context.currentCheque - totalCost;
            
        case CurrencyType::Gaya:
            if (m_context.currentGaya < totalCost) {
                return std::unexpected(Core::CommandError::InvalidParameter);
            }
            return m_context.currentGaya - totalCost;
            
        default:
            return std::unexpected(Core::CommandError::InvalidParameter);
    }
}

} // namespace Client::Gameplay
