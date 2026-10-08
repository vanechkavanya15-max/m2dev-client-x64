#include <cassert>
#include <iostream>
#include <string_view>
#include <format>

#include "Client/Core/DomainErrors.h"
#include "Client/Core/DomainEvents.h"
#include "Client/Gameplay/InventoryDomain.h"
#include "Client/Gameplay/SkillDomain.h"
#include "UserInterface/Core/EventBus.h"
#include "Client/Gameplay/CombatError.h"
#include "UserInterface/ExchangeErrors.h"
#include "UserInterface/GuildErrors.h"
#include "EterBase/Result.h"

using namespace Client::Core;
using namespace Client::Gameplay;
using CoreInventorySlotUpdatedEvent = Client::Core::InventorySlotUpdatedEvent;

void TestDomainErrorsEnumAndToString() {
    // InventoryError
    assert(to_string(InventoryError::None) == "InventoryError::None");
    assert(to_string(InventoryError::SlotOutOfBounds) == "InventoryError::SlotOutOfBounds");
    assert(to_string(InventoryError::SlotOccupied).find("occupied") != std::string_view::npos);
    assert(to_string(InventoryError::SlotEmpty).find("empty") != std::string_view::npos);
    assert(to_string(InventoryError::ItemLocked).find("locked") != std::string_view::npos);
    assert(to_string(InventoryError::InsufficientCount).find("count") != std::string_view::npos);
    assert(to_string(InventoryError::InvalidVnum).find("invalid") != std::string_view::npos);

    // SkillError
    assert(to_string(SkillError::None) == "SkillError::None");
    assert(to_string(SkillError::SkillNotFound) == "SkillError::SkillNotFound");
    assert(to_string(SkillError::NotEnoughSP) == "SkillError::NotEnoughSP");
    assert(to_string(SkillError::OnCooldown) == "SkillError::OnCooldown");
    assert(to_string(SkillError::RequirementNotMet) == "SkillError::RequirementNotMet");

    // ActorError
    assert(to_string(ActorError::None) == "ActorError::None");
    assert(to_string(ActorError::ActorNotFound) == "ActorError::ActorNotFound");
    assert(to_string(ActorError::AlreadyDead) == "ActorError::AlreadyDead");
    assert(to_string(ActorError::InvalidPosition) == "ActorError::InvalidPosition");

    // std::format support
    std::string s1 = std::format("{}", InventoryError::SlotOccupied);
    assert(!s1.empty());
    std::string s2 = std::format("{}", SkillError::NotEnoughSP);
    assert(!s2.empty());
    std::string s3 = std::format("{}", ActorError::AlreadyDead);
    assert(!s3.empty());
 
    // Filar 4: Testy nowych bledow domenowych
    assert(Client::Gameplay::ToString(Client::Gameplay::CombatError::TargetDead) == "Target is dead");
    assert(Client::Gameplay::ToString(Client::Gameplay::CombatError::CharacterStunned) == "Character is stunned");
    assert(UserInterface::Exchange::to_string(UserInterface::Exchange::ExchangeError::InvalidPosition) == "ExchangeError::InvalidPosition");
    assert(to_string(GuildError::NotAuthorized).find("Not authorized") != std::string_view::npos);
    assert(EterBase::ToString(EterBase::PacketDispatchError::Disconnected) == "Disconnected");
    assert(std::format("{}", EterBase::PacketDispatchError::QueueFull) == "QueueFull");

    std::cout << "[PASS] TestDomainErrorsEnumAndToString\n";
}

void TestDomainEventsStructures() {
    // InventorySlotUpdatedEvent
    CoreInventorySlotUpdatedEvent invEv1{5, 1001, 10};
    assert(invEv1.slot == 5);
    assert(invEv1.vnum == 1001);
    assert(invEv1.count == 10);
    CoreInventorySlotUpdatedEvent invEv2{5, 1001, 10};
    assert(invEv1 == invEv2);

    // SkillCooldownStartedEvent
    SkillCooldownStartedEvent skillEv1{42, 5000};
    assert(skillEv1.skillId == 42);
    assert(skillEv1.durationMs == 5000);
    SkillCooldownStartedEvent skillEv2{42, 5000};
    assert(skillEv1 == skillEv2);

    // ActorDeadEvent
    ActorDeadEvent actorEv1{999};
    assert(actorEv1.vid == 999);
    ActorDeadEvent actorEv2{999};
    assert(actorEv1 == actorEv2);

    // PlayerGoldUpdatedEvent
    PlayerGoldUpdatedEvent goldEv1{100, 250};
    assert(goldEv1.oldGold == 100);
    assert(goldEv1.newGold == 250);
    PlayerGoldUpdatedEvent goldEv2{100, 250};
    assert(goldEv1 == goldEv2);

    std::cout << "[PASS] TestDomainEventsStructures\n";
}

void TestInventoryDomainErrorsAndEvents() {
    InventoryDomain inv;

    bool eventReceived = false;
    uint16_t lastEventSlot = 0xFFFF;
    uint32_t lastEventVnum = 0;

    auto subId = UserInterface::Core::EventBus::GetInstance().Subscribe<CoreInventorySlotUpdatedEvent>(
        [&](const CoreInventorySlotUpdatedEvent& ev) {
            eventReceived = true;
            lastEventSlot = ev.slot;
            lastEventVnum = ev.vnum;
        }
    );

    ItemData item1;
    item1.vnum = EterBase::ItemVnum(101);
    item1.count = 5;
    item1.size = {1, 1};

    // Dodanie itemu z uzyciem AddItem (Result<void, InventoryError>)
    auto addRes1 = inv.AddItem(InventoryWindow::Inventory, EterBase::ItemSlot(10), item1);
    assert(addRes1.has_value());
    assert(eventReceived);
    assert(lastEventSlot == 10);
    assert(lastEventVnum == 101);

    // Proba dodania na ten sam slot - powienien zwrocic SlotOccupied
    auto addResDup = inv.AddItem(InventoryWindow::Inventory, EterBase::ItemSlot(10), item1);
    assert(!addResDup.has_value());
    assert(addResDup.error() == InventoryError::SlotOccupied);

    // Proba dodania poza zakres - SlotOutOfBounds
    auto addResOut = inv.AddItem(InventoryWindow::Inventory, EterBase::ItemSlot(9999), item1);
    assert(!addResOut.has_value());
    assert(addResOut.error() == InventoryError::SlotOutOfBounds);

    // Dodanie automatyczne AddItem(item) do pierwszego wolnego slotu
    ItemData item2;
    item2.vnum = EterBase::ItemVnum(202);
    item2.count = 1;
    item2.size = {1, 1};
    auto addAutoRes = inv.AddItem(item2);
    assert(addAutoRes.has_value());

    // Usuniecie z pustego slotu - SlotEmpty
    auto remEmptyRes = inv.RemoveItemResult(InventoryWindow::Inventory, EterBase::ItemSlot(15));
    assert(!remEmptyRes.has_value());
    assert(remEmptyRes.error() == InventoryError::SlotEmpty);

    // Poprawne usuniecie z zajetego slotu (slot 10)
    eventReceived = false;
    auto remOkRes = inv.RemoveItemResult(InventoryWindow::Inventory, EterBase::ItemSlot(10));
    assert(remOkRes.has_value());
    assert(eventReceived);
    assert(lastEventSlot == 10);
    assert(lastEventVnum == 0); // Po usunieciu vnum == 0

    // Ponowne usuniecie tego samego slotu - SlotEmpty
    auto remAgainRes = inv.RemoveItem(10);
    assert(!remAgainRes.has_value());
    assert(remAgainRes.error() == InventoryError::SlotEmpty);

    UserInterface::Core::EventBus::GetInstance().Unsubscribe<CoreInventorySlotUpdatedEvent>(subId);
    std::cout << "[PASS] TestInventoryDomainErrorsAndEvents\n";
}

void TestSkillDomainErrorsAndEvents() {
    SkillDomain skillDomain;

    bool cooldownEventReceived = false;
    uint32_t lastSkillId = 0;
    uint32_t lastDurationMs = 0;

    auto subId = UserInterface::Core::EventBus::GetInstance().Subscribe<SkillCooldownStartedEvent>(
        [&](const SkillCooldownStartedEvent& ev) {
            cooldownEventReceived = true;
            lastSkillId = ev.skillId;
            lastDurationMs = ev.durationMs;
        }
    );

    // Skill jeszcze niezdefiniowany ani niezarejestrowany: CanCast -> SkillNotFound
    auto canCastNotFound = skillDomain.CanCast(1, 100);
    assert(!canCastNotFound.has_value());
    assert(canCastNotFound.error() == SkillError::SkillNotFound);

    // CalculateSPCostResult dla nieznanego skilla -> SkillNotFound
    auto spCostNotFound = skillDomain.CalculateSPCostResult(1);
    assert(!spCostNotFound.has_value());
    assert(spCostNotFound.error() == SkillError::SkillNotFound);

    // Definiujemy skill
    SkillData fireball;
    fireball.id = 1;
    fireball.type = SkillType::Active;
    fireball.baseSPCost = 50;
    fireball.spMultiplier = 2.0f;
    fireball.name = "Fireball";
    skillDomain.DefineSkill(1, fireball);
    skillDomain.RegisterSkill(1, 10); // Poziom 10 -> koszt 50 + 10 * 2.0 = 70 SP

    // CalculateSPCostResult poprawny
    auto spCostRes = skillDomain.CalculateSPCostResult(1);
    assert(spCostRes.has_value());
    assert(spCostRes.value() == 70);

    // CalculateSPCostSafe poprawny
    auto spSafeRes = skillDomain.CalculateSPCostSafe(1);
    assert(spSafeRes.has_value());
    assert(spSafeRes.value() == 70);

    // CanCast przy za malej ilosci SP (np. 40 SP gdy koszt to 70) -> NotEnoughSP
    auto canCastLowSP = skillDomain.CanCast(1, 40);
    assert(!canCastLowSP.has_value());
    assert(canCastLowSP.error() == SkillError::NotEnoughSP);

    // CanCast przy wystarczajacej ilosci SP (np. 100 SP) -> Sukces
    auto canCastOk = skillDomain.CanCast(1, 100);
    assert(canCastOk.has_value());

    // CanCast bez podawania SP (tylko weryfikacja istnienia i braku cooldownu) -> Sukces
    auto canCastNoSPParam = skillDomain.CanCast(1);
    assert(canCastNoSPParam.has_value());

    // Rozpoczecie cooldownu (5000 ms) -> publikacja SkillCooldownStartedEvent
    skillDomain.StartCooldown(1, 5000);
    assert(cooldownEventReceived);
    assert(lastSkillId == 1);
    assert(lastDurationMs == 5000);

    // Teraz CanCast powinien zwrocic OnCooldown
    auto canCastCooling = skillDomain.CanCast(1, 100);
    assert(!canCastCooling.has_value());
    assert(canCastCooling.error() == SkillError::OnCooldown);

    // Po resecie cooldownu CanCast znowu powinien byc sukcesem
    skillDomain.ResetCooldown(1);
    auto canCastReset = skillDomain.CanCast(1, 100);
    assert(canCastReset.has_value());

    UserInterface::Core::EventBus::GetInstance().Unsubscribe<SkillCooldownStartedEvent>(subId);
    std::cout << "[PASS] TestSkillDomainErrorsAndEvents\n";
}

int main() {
    std::cout << "--- Rozpoczynanie testow Domain Errors & Domain Events ---\n";
    TestDomainErrorsEnumAndToString();
    TestDomainEventsStructures();
    TestInventoryDomainErrorsAndEvents();
    TestSkillDomainErrorsAndEvents();
    std::cout << "--- Wszystkie testy Domain Errors & Domain Events zakonczone sukcesem! ---\n";
    return 0;
}
