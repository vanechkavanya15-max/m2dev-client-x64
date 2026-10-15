#include "../StdAfx.h"
#include "IGroundDropBatchRenderer.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Core/Events/ItemEvents.h"

#include <string_view>
#include <string>

namespace UserInterface::GroundDrop
{

/**
 * @brief Weryfikacja prawa wlasnosci do lezacego dropu (ochrona przed kradzieza).
 */
std::expected<void, EterBase::EntityError> VerifyDropOwnership(const GroundDropItemData& item, EterBase::EntityId playerVid)
{
    if (playerVid.value() == 0)
    {
        EterBase::ModernLogger::Error("VerifyDropOwnership: Odrzucono walidacje, nieprawidlowy EntityId gracza.");
        return std::unexpected(EterBase::EntityError::NotFound);
    }

    if (item.virtualId == 0)
    {
        EterBase::ModernLogger::Error("VerifyDropOwnership: Odrzucono walidacje, nieprawidlowy identyfikator dropu.");
        return std::unexpected(EterBase::EntityError::NotFound);
    }

    // Brak wlasciciela - kazdy moze podniesc
    if (item.ownerVid == 0)
    {
        return {};
    }

    // Wlasciciel zgadza sie z graczem
    if (item.ownerVid == playerVid.value())
    {
        return {};
    }

    // Proba kradziezy (lub uplynela waznosc z punktu widzenia UI)
    EterBase::ModernLogger::Warning(
        "VerifyDropOwnership: Gracz {} probowal podniesc przedmiot {}, ktorego wlascicielem jest {}.", 
        playerVid.value(), item.virtualId, item.ownerVid);

    return std::unexpected(EterBase::EntityError::OutOfRange); 
}

/**
 * @brief Aktualizacja wlasnosci dropu i powiadomienie przez szyne zdarzen.
 */
std::expected<void, EterBase::EntityError> UpdateDropOwnership(GroundDropItemData& item, EterBase::EntityId newOwnerVid, std::string_view newOwnerName)
{
    if (item.virtualId == 0)
    {
        EterBase::ModernLogger::Error("UpdateDropOwnership: Nieprawidlowy identyfikator dropu.");
        return std::unexpected(EterBase::EntityError::NotFound);
    }

    item.ownerVid = newOwnerVid.value();

    EterBase::ModernLogger::Info(
        "UpdateDropOwnership: Zaktualizowano wlasnosc przedmiotu {} na gracza {} ({}).", 
        item.virtualId, newOwnerVid.value(), newOwnerName);

    auto eventResult = UserInterface::Core::Events::ItemOwnershipChanged::Create(EterBase::EntityId(item.virtualId), std::string(newOwnerName));
    if (eventResult.has_value())
    {
        UserInterface::Core::EventBus::GetInstance().Publish(eventResult.value());
    }
    else
    {
        EterBase::ModernLogger::Error("UpdateDropOwnership: Nie udalo sie utworzyc zdarzenia ItemOwnershipChanged.");
        return std::unexpected(eventResult.error());
    }

    return {};
}

} // namespace UserInterface::GroundDrop
