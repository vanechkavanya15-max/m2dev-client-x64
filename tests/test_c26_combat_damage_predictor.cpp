#include "../src/Client/Gameplay/CombatDamagePredictor.h"
#include <iostream>
#include <cassert>

using namespace Client::Gameplay;

void TestPredictDamage_BaseDamage() {
    CombatDamagePredictor predictor;
    AttackerStats attacker = { 100, 0, 0 };
    TargetStats defender = { 20, 0, 0 };
    
    auto result = predictor.PredictDamage(attacker, defender);
    assert(result.has_value());
    assert(result.value().damage == 80);
    assert(!result.value().isCritical && !result.value().isPierce && !result.value().isBlock && !result.value().isDodge);
    std::cout << "TestPredictDamage_BaseDamage passed.\n";
}

void TestPredictDamage_Dodge() {
    CombatDamagePredictor predictor;
    AttackerStats attacker = { 100, 0, 0 };
    TargetStats defender = { 20, 100, 0 }; // 100% dodge
    
    auto result = predictor.PredictDamage(attacker, defender);
    assert(result.has_value());
    assert(result.value().damage == 0);
    assert(result.value().isDodge);
    std::cout << "TestPredictDamage_Dodge passed.\n";
}

void TestPredictDamage_Block() {
    CombatDamagePredictor predictor;
    AttackerStats attacker = { 100, 0, 0 };
    TargetStats defender = { 20, 0, 100 }; // 100% block
    
    auto result = predictor.PredictDamage(attacker, defender);
    assert(result.has_value());
    assert(result.value().damage == 0);
    assert(result.value().isBlock);
    std::cout << "TestPredictDamage_Block passed.\n";
}

void TestPredictDamage_Pierce() {
    CombatDamagePredictor predictor;
    AttackerStats attacker = { 100, 0, 100 }; // 100% pierce
    TargetStats defender = { 50, 0, 0 };
    
    auto result = predictor.PredictDamage(attacker, defender);
    assert(result.has_value());
    assert(result.value().damage == 100);
    assert(result.value().isPierce);
    std::cout << "TestPredictDamage_Pierce passed.\n";
}

void TestPredictDamage_Critical() {
    CombatDamagePredictor predictor;
    AttackerStats attacker = { 100, 100, 0 }; // 100% critical
    TargetStats defender = { 20, 0, 0 };
    
    auto result = predictor.PredictDamage(attacker, defender);
    assert(result.has_value());
    assert(result.value().damage == 160); // (100 - 20) * 2 = 160
    assert(result.value().isCritical);
    std::cout << "TestPredictDamage_Critical passed.\n";
}

void TestPredictDamage_InvalidParameters() {
    CombatDamagePredictor predictor;
    AttackerStats attacker = { 100, 101, 0 }; 
    TargetStats defender = { 20, 0, 0 };
    
    auto result = predictor.PredictDamage(attacker, defender);
    assert(!result.has_value());
    assert(result.error() == Client::Core::CommandError::InvalidParameter);
    std::cout << "TestPredictDamage_InvalidParameters passed.\n";
}

int main() {
    TestPredictDamage_BaseDamage();
    TestPredictDamage_Dodge();
    TestPredictDamage_Block();
    TestPredictDamage_Pierce();
    TestPredictDamage_Critical();
    TestPredictDamage_InvalidParameters();
    
    std::cout << "All tests passed!\n";
    return 0;
}
