#include "../StdAfx.h"
#include "IFastItemTooltipCache.h"
#include "../Core/EventBus.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"

#include <format>
#include <string>
#include <expected>

namespace UserInterface::GroundDrop {

namespace {

    /**
     * @brief Zdarzenie domenowe emitowane po aktualizacji tooltipa o bonusy zestawu.
     */
    struct SetBonusTooltipUpdatedEvent : public UserInterface::Core::IEvent {
        EterBase::ItemVnum vnum;

        explicit SetBonusTooltipUpdatedEvent(EterBase::ItemVnum vnum) : vnum(vnum) {}
    };

    /**
     * @brief Pomocnicza funkcja pobierajaca opis bonusu zestawu (Mock dla architektury).
     */
    std::string GetSetBonusDescription(EterBase::ItemVnum vnum) {
        // Przykladowa logika dla konkretnego przedmiotu (np. zbroi z zestawu)
        if (vnum.value() == 11299) {
            return "|cFF00FF00[Set Bonus Active]|r\nMax HP +2000\nAttack Value +50";
        }
        return "";
    }

} // namespace

/**
 * @brief Aktualizuje zbuforowany tooltip przedmiotu dodajac informacje o bonusach zestawu (Set Bonus).
 *
 * @param tooltipCache Referencja do cache'u tooltipow.
 * @param vnum Vnum przedmiotu do zaktualizowania.
 * @return std::expected<void, EterBase::EntityError> Sukces lub blad w przypadku braku tooltipa.
 */
[[nodiscard]] std::expected<void, EterBase::EntityError> AppendSetBonusToTooltip(IFastItemTooltipCache& tooltipCache, EterBase::ItemVnum vnum) {
    auto tooltipOpt = tooltipCache.GetTooltip(vnum);
    if (!tooltipOpt.has_value()) {
        EterBase::ModernLogger::Log(EterBase::LogLevel::Warning, "Tooltip SetBonus: Tooltip not found in cache for VNUM {}", vnum.value());
        return std::unexpected(EterBase::EntityError::NotFound);
    }

    std::string setBonusText = GetSetBonusDescription(vnum);
    if (setBonusText.empty()) {
        EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "Tooltip SetBonus: No set bonus for VNUM {}", vnum.value());
        return {}; // Brak bonusu, nie ma potrzeby aktualizacji
    }

    FormattedTooltipData updatedData = tooltipOpt.value();
    updatedData.description = std::format("{}\n\n{}", updatedData.description, setBonusText);

    tooltipCache.CacheTooltip(vnum, updatedData);

    EterBase::ModernLogger::Log(EterBase::LogLevel::Info, "Tooltip SetBonus: Appended set bonus to VNUM {}", vnum.value());

    UserInterface::Core::EventBus::GetInstance().Publish(SetBonusTooltipUpdatedEvent{vnum});

    return {};
}

} // namespace UserInterface::GroundDrop
