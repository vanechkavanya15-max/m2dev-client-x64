#include "../StdAfx.h"
#include <cstdint>
#include <string>
#include <format>
#include <expected>

#include "IFastItemTooltipCache.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace UserInterface::GroundDrop::ReqValidator
{
    // Define the required flags locally for validation to prevent including big headers
    enum EItemAntiFlag
    {
        ITEM_ANTIFLAG_FEMALE        = (1 << 0),     // 여성 사용 불가
        ITEM_ANTIFLAG_MALE          = (1 << 1),     // 남성 사용 불가
        ITEM_ANTIFLAG_WARRIOR       = (1 << 2),     // 무사 사용 불가
        ITEM_ANTIFLAG_ASSASSIN      = (1 << 3),     // 자객 사용 불가
        ITEM_ANTIFLAG_SURA          = (1 << 4),     // 수라 사용 불가 
        ITEM_ANTIFLAG_SHAMAN        = (1 << 5),     // 무당 사용 불가
    };

    struct TooltipReqValidationEvent : public Core::IEvent
    {
        EterBase::ItemVnum vnum;
        bool bLevelMet;
        bool bRaceMet;
        bool bSexMet;

        TooltipReqValidationEvent(EterBase::ItemVnum vnum, bool bLevel, bool bRace, bool bSex)
            : vnum(vnum), bLevelMet(bLevel), bRaceMet(bRace), bSexMet(bSex) {}
    };

    /**
     * @brief Validates item requirements (Level, Class, Sex) and updates tooltip description.
     * @param vnum The item vnum.
     * @param playerLevel The level of the player.
     * @param playerJob The job/class index of the player (0=Warrior, 1=Assassin, 2=Sura, 3=Shaman).
     * @param playerSex The gender index of the player (1=Male, 0=Female).
     * @param itemLimitLevel The level limit required to use the item.
     * @param itemAntiFlags The item anti-flags restricting usage.
     * @param outData The tooltip data to be updated with requirement warnings if any fail.
     * @return EterBase::Result<void, EterBase::EntityError> Success if validation runs.
     */
    EterBase::Result<void, EterBase::EntityError> ValidateItemRequirements(
        EterBase::ItemVnum vnum,
        uint8_t playerLevel,
        uint8_t playerJob,
        uint8_t playerSex,
        uint32_t itemLimitLevel,
        uint32_t itemAntiFlags,
        FormattedTooltipData& outData)
    {
        bool bLevelMet = (playerLevel >= itemLimitLevel);

        bool bWarrior = !(itemAntiFlags & ITEM_ANTIFLAG_WARRIOR);
        bool bAssassin = !(itemAntiFlags & ITEM_ANTIFLAG_ASSASSIN);
        bool bSura = !(itemAntiFlags & ITEM_ANTIFLAG_SURA);
        bool bShaman = !(itemAntiFlags & ITEM_ANTIFLAG_SHAMAN);

        bool canUseRace = false;
        if (playerJob == 0 && bWarrior) canUseRace = true;
        if (playerJob == 1 && bAssassin) canUseRace = true;
        if (playerJob == 2 && bSura) canUseRace = true;
        if (playerJob == 3 && bShaman) canUseRace = true;

        bool bMale = !(itemAntiFlags & ITEM_ANTIFLAG_MALE);
        bool bFemale = !(itemAntiFlags & ITEM_ANTIFLAG_FEMALE);

        bool canUseSex = false;
        if (playerSex == 1 && bMale) canUseSex = true;
        if (playerSex == 0 && bFemale) canUseSex = true;

        if (!bLevelMet)
        {
            outData.description += std::format("\n|cffff0000Required Level: {}|r", itemLimitLevel);
        }

        if (!canUseRace)
        {
            outData.description += "\n|cffff0000Class requirement not met|r";
        }

        if (!canUseSex)
        {
            outData.description += "\n|cffff0000Gender requirement not met|r";
        }
        
        EterBase::ModernLogger::Debug("TooltipReqValidate: Vnum {} Level: {} Race: {} Sex: {}", vnum.value(), bLevelMet, canUseRace, canUseSex);

        Core::EventBus::GetInstance().Publish(TooltipReqValidationEvent{vnum, bLevelMet, canUseRace, canUseSex});
        return {};
    }
}
