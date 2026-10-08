#pragma once

#include "../../StdAfx.h"
#include <cstdint>
#include <expected>
#include "../Core/WorldContext.h"
#include "../Core/DomainCommands.h"
#include "CurrencyType.h"

namespace Client::Gameplay {

class ShopPriceValidator {
public:
    explicit ShopPriceValidator(const Core::WorldContext& context) noexcept;

    // Zwraca pozostaly bilans w Result lub CommandError
    [[nodiscard]] std::expected<int64_t, Core::CommandError> ValidatePurchase(
        CurrencyType currency,
        int64_t unitPrice,
        uint32_t count) const noexcept;

private:
    const Core::WorldContext& m_context;
};

} // namespace Client::Gameplay
