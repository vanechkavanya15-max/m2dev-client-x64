#include "../src/Client/Gameplay/SkillDomain.h"
#include <cassert>
#include <iostream>
#include <thread>

using namespace Client::Gameplay;

void TestSkillRegistration() {
    SkillDomain domain;
    
    // Brak skilla
    assert(!domain.HasSkill(1));
    assert(domain.GetSkillLevel(1) == 0);

    // Rejestracja bez poziomu
    domain.RegisterSkill(1);
    assert(domain.HasSkill(1));
    assert(domain.GetSkillLevel(1) == 0);
    assert(domain.GetSkillMasterType(1) == 0);
    assert(domain.GetSkillMasteryLevel(1) == 0);

    // Ustawianie poziomu normalnego (10)
    domain.SetSkillLevel(1, 10);
    assert(domain.GetSkillLevel(1) == 10);
    assert(domain.GetSkillMasterType(1) == 0);
    assert(domain.GetSkillMasteryLevel(1) == 10);

    // Rejestracja z poziomem mistrzowskim M2 (21)
    domain.RegisterSkill(2, 21); // M2
    assert(domain.HasSkill(2));
    assert(domain.GetSkillLevel(2) == 21);
    assert(domain.GetSkillMasterType(2) == 1);
    assert(domain.GetSkillMasteryLevel(2) == 2);

    // Jawne ustawianie mistrzostwa G5 (GrandMaster 5)
    domain.RegisterSkill(3);
    domain.SetSkillMastery(3, 2, 5);
    assert(domain.GetSkillMasterType(3) == 2);
    assert(domain.GetSkillMasteryLevel(3) == 5);
    assert(domain.GetSkillLevel(3) == 34);

    // Rejestracja ze wszystkimi parametrami P (PerfectMaster 1)
    domain.RegisterSkill(4, 40, 3, 1);
    assert(domain.GetSkillMasterType(4) == 3);
    assert(domain.GetSkillMasteryLevel(4) == 1);
    const auto* state4 = domain.GetSkillState(4);
    assert(state4 != nullptr);
    assert(state4->masterType == 3);
    assert(state4->masteryLevel == 1);

    std::cout << "TestSkillRegistration passed.\n";
}

void TestSkillCooldown() {
    SkillDomain domain;
    domain.RegisterSkill(5, 1);
    
    // Zaraz po rejestracji (bez aktywnego cooldownu) jest gotowe
    assert(domain.IsSkillReady(5));
    assert(domain.GetRemainingCooldown(5).count() == 0);
    assert(domain.GetCooldownProgress(5) == 1.0f);
    
    // Ustawiamy cooldown na 100 ms
    domain.StartCooldown(5, std::chrono::milliseconds(100));
    assert(!domain.IsSkillReady(5));
    assert(domain.GetRemainingCooldown(5).count() > 0);
    assert(domain.GetRemainingCooldownMs(5) > 0);
    assert(domain.GetCooldownTracker().IsOnCooldown(5));
    
    // Czekamy na uplyniecie czesci czasu
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(!domain.IsSkillReady(5));

    // Czekamy na uplyniecie reszty
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    domain.UpdateCooldowns(); // Opcjonalne do zmiany flag, ale IsSkillReady samo sprawdza czas
    
    assert(domain.IsSkillReady(5));
    assert(domain.GetRemainingCooldown(5).count() == 0);
    assert(domain.GetCooldownProgress(5) == 1.0f);
    
    // Testowanie resetowania
    domain.StartCooldown(5, std::chrono::milliseconds(1000));
    assert(!domain.IsSkillReady(5));
    domain.ResetCooldown(5);
    assert(domain.IsSkillReady(5));

    // Testowanie ResetAllCooldowns
    domain.StartCooldown(5, std::chrono::milliseconds(1000));
    assert(!domain.IsSkillReady(5));
    domain.ResetAllCooldowns();
    assert(domain.IsSkillReady(5));

    std::cout << "TestSkillCooldown passed.\n";
}

void TestSPCostCalculation() {
    SkillDomain domain;
    
    SkillData auraData;
    auraData.id = 10;
    auraData.type = SkillType::Toggle;
    auraData.baseSPCost = 100;
    auraData.spMultiplier = 5.0f;
    domain.DefineSkill(10, auraData);

    domain.RegisterSkill(10, 5); // Poziom 5 (Normal) - Mnożnik mistrzowski 1.0
    
    uint32_t cost = domain.CalculateSPCost(10);
    // 100 + (5 * 5.0 * 1.0) = 125
    assert(cost == 125); 

    domain.SetSkillLevel(10, 25); // Poziom 25 (Master) - Mnożnik mistrzowski 1.5
    cost = domain.CalculateSPCost(10);
    // 100 + (25 * 5.0 * 1.5) = 100 + 187.5 = 287
    assert(cost == 287);

    domain.SetSkillLevel(10, 35); // Poziom 35 (GrandMaster) - Mnożnik mistrzowski 2.0
    cost = domain.CalculateSPCost(10);
    // 100 + (35 * 5.0 * 2.0) = 100 + 350 = 450
    assert(cost == 450);

    domain.SetSkillLevel(10, 45); // Poziom 45 (PerfectMaster) - Mnożnik mistrzowski 2.5
    cost = domain.CalculateSPCost(10);
    // 100 + (45 * 5.0 * 2.5) = 100 + 562.5 = 662
    assert(cost == 662);

    std::cout << "TestSPCostCalculation passed.\n";
}

void TestToggleSkill() {
    SkillDomain domain;
    
    SkillData activeSkill;
    activeSkill.id = 1;
    activeSkill.type = SkillType::Active;
    domain.DefineSkill(1, activeSkill);

    SkillData toggleSkill;
    toggleSkill.id = 2;
    toggleSkill.type = SkillType::Toggle;
    domain.DefineSkill(2, toggleSkill);

    domain.RegisterSkill(1, 10);
    domain.RegisterSkill(2, 10);

    // Umiejętność Active nie powinna dać się przełączyć (toggle)
    domain.ToggleSkill(1, true);
    assert(!domain.IsSkillToggledOn(1));

    // Umiejętność Toggle powinna dać się przełączyć
    assert(!domain.IsSkillToggledOn(2));
    domain.ToggleSkill(2, true);
    assert(domain.IsSkillToggledOn(2));
    
    domain.ToggleSkill(2, false);
    assert(!domain.IsSkillToggledOn(2));

    std::cout << "TestToggleSkill passed.\n";
}

void TestGetSkillData() {
    SkillDomain domain;
    
    SkillData auraData;
    auraData.id = 99;
    auraData.name = "Aura Miecza";
    domain.DefineSkill(99, auraData);

    const SkillData* data = domain.GetSkillData(99);
    assert(data != nullptr);
    assert(data->id == 99);
    assert(data->name == "Aura Miecza");

    const SkillData* missingData = domain.GetSkillData(100);
    assert(missingData == nullptr);

    std::cout << "TestGetSkillData passed.\n";
}

int main() {
    TestSkillRegistration();
    TestSkillCooldown();
    TestSPCostCalculation();
    TestToggleSkill();
    TestGetSkillData();
    std::cout << "All SkillDomain tests passed successfully.\n";
    return 0;
}
