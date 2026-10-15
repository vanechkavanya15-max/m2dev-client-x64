#include <iostream>
#include <cassert>
#include <string>
#include "../src/Client/Network/ServerProfile.h"

using namespace Client::Network;

void TestBasicSettersAndGetters() {
    ServerProfile profile;
    
    profile.SetHost("127.0.0.1");
    assert(profile.GetHost() == "127.0.0.1");
    
    profile.SetPort(50000);
    assert(profile.GetPort() == 50000);
    
    profile.SetProfileName("LocalTest");
    assert(profile.GetProfileName() == "LocalTest");
    
    profile.SetFramingMode(FramingMode::M2Dev4B);
    assert(profile.GetFramingMode() == FramingMode::M2Dev4B);
    
    profile.SetCryptoType(CryptoType::Sodium);
    assert(profile.GetCryptoType() == CryptoType::Sodium);
    
    profile.SetMoveInterval(200);
    assert(profile.GetMoveInterval() == 200);
    
    profile.SetPickupInterval(500);
    assert(profile.GetPickupInterval() == 500);
    
    profile.SetAttackCrcRequired(true);
    assert(profile.IsAttackCrcRequired() == true);
    
    std::cout << "[OK] TestBasicSettersAndGetters passed.\n";
}

void TestHeaderSizeConstraints() {
    ServerProfile profile;
    
    // Default should be 1
    assert(profile.GetHeaderSizeBytes() == 1);
    
    // Valid 1
    auto res1 = profile.SetHeaderSizeBytes(1);
    assert(res1.has_value());
    assert(profile.GetHeaderSizeBytes() == 1);
    
    // Valid 2
    auto res2 = profile.SetHeaderSizeBytes(2);
    assert(res2.has_value());
    assert(profile.GetHeaderSizeBytes() == 2);
    
    // Invalid
    auto res3 = profile.SetHeaderSizeBytes(3);
    assert(!res3.has_value());
    assert(res3.error() == "Header size must be 1 or 2 bytes");
    assert(profile.GetHeaderSizeBytes() == 2); // Should remain unchanged
    
    auto res4 = profile.SetHeaderSizeBytes(0);
    assert(!res4.has_value());
    assert(res4.error() == "Header size must be 1 or 2 bytes");
    assert(profile.GetHeaderSizeBytes() == 2); // Should remain unchanged
    
    std::cout << "[OK] TestHeaderSizeConstraints passed.\n";
}

void TestOpcodeMapping() {
    ServerProfile profile;
    
    profile.RegisterOpcode("CG_MOVE", 10);
    profile.RegisterOpcode("CG_ATTACK", 11);
    
    auto moveRes = profile.GetOpcode("CG_MOVE");
    assert(moveRes.has_value());
    assert(moveRes.value() == 10);
    
    auto attackRes = profile.GetOpcode("CG_ATTACK");
    assert(attackRes.has_value());
    assert(attackRes.value() == 11);
    
    auto invalidRes = profile.GetOpcode("CG_UNKNOWN");
    assert(!invalidRes.has_value());
    assert(invalidRes.error() == "Opcode not found");
    
    // Overwrite test
    profile.RegisterOpcode("CG_MOVE", 20);
    auto moveRes2 = profile.GetOpcode("CG_MOVE");
    assert(moveRes2.has_value());
    assert(moveRes2.value() == 20);
    
    std::cout << "[OK] TestOpcodeMapping passed.\n";
}

// Dummy StdAfx for testing standalone
namespace {
    struct MockStdAfx {
        MockStdAfx() {
            // Setup mock environment if needed
        }
    } mockStdAfx;
}

int main() {
    std::cout << "Running ServerProfile tests...\n";
    
    TestBasicSettersAndGetters();
    TestHeaderSizeConstraints();
    TestOpcodeMapping();
    
    std::cout << "All tests passed successfully.\n";
    return 0;
}
