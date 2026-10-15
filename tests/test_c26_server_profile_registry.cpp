#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "../src/Client/Network/ServerProfileRegistry.h"
#include "../src/Client/Network/ServerProfile.cpp" // Include cpp for simple compilation
#include "../src/Client/Network/ServerProfileRegistry.cpp"
#include "../src/Client/Core/DomainCommands.h"

void test_load_and_active_profile() {
    std::filesystem::path testDir = "temp_test_profiles_dir";
    std::filesystem::create_directory(testDir);

    std::ofstream out1(testDir / "profile1.json");
    out1 << R"({
        "name": "Server1",
        "host": "127.0.0.1",
        "port": 50000,
        "framing": "Ymir1B",
        "crypto": "AES",
        "opcodes": {
            "LOGIN": 1,
            "ATTACK": 2
        }
    })";
    out1.close();

    std::ofstream out2(testDir / "profile2.json");
    out2 << R"({
        "name": "Server2",
        "host": "192.168.0.1",
        "port": 50010,
        "framing": "M2Dev4B",
        "crypto": "TEA",
        "opcodes": {
            "MOVE": 3
        }
    })";
    out2.close();

    Client::Network::ServerProfileRegistry registry;
    auto result = registry.LoadAllFromDirectory(testDir);
    assert(result.has_value());

    auto setActiveResult1 = registry.SetActiveProfile("Server1");
    assert(setActiveResult1.has_value());

    auto activeProfile1 = registry.GetActiveProfile();
    assert(activeProfile1.has_value());
    assert(activeProfile1->GetProfileName() == "Server1");
    assert(activeProfile1->GetHost() == "127.0.0.1");
    assert(activeProfile1->GetPort() == 50000);
    assert(activeProfile1->GetFramingMode() == Client::Network::FramingMode::Ymir1B);
    assert(activeProfile1->GetCryptoType() == Client::Network::CryptoType::AES);
    assert(activeProfile1->GetOpcode("LOGIN").value() == 1);
    assert(activeProfile1->GetOpcode("ATTACK").value() == 2);

    auto setActiveResult2 = registry.SetActiveProfile("Server2");
    assert(setActiveResult2.has_value());

    auto activeProfile2 = registry.GetActiveProfile();
    assert(activeProfile2.has_value());
    assert(activeProfile2->GetProfileName() == "Server2");
    assert(activeProfile2->GetHost() == "192.168.0.1");
    assert(activeProfile2->GetPort() == 50010);
    assert(activeProfile2->GetFramingMode() == Client::Network::FramingMode::M2Dev4B);
    assert(activeProfile2->GetCryptoType() == Client::Network::CryptoType::TEA);
    assert(activeProfile2->GetOpcode("MOVE").value() == 3);
    
    auto getOpcodeFail = activeProfile2->GetOpcode("INVALID");
    assert(!getOpcodeFail.has_value());

    std::filesystem::remove_all(testDir);
}

void test_invalid_directory() {
    Client::Network::ServerProfileRegistry registry;
    auto result = registry.LoadAllFromDirectory("non_existent_dir");
    assert(!result.has_value());
    assert(result.error() == Client::Core::CommandError::InvalidParameter);
}

void test_set_invalid_profile() {
    Client::Network::ServerProfileRegistry registry;
    auto setActiveResult = registry.SetActiveProfile("NonExistent");
    assert(!setActiveResult.has_value());
    assert(setActiveResult.error() == Client::Core::CommandError::InvalidParameter);
}

void test_get_without_active() {
    Client::Network::ServerProfileRegistry registry;
    auto getResult = registry.GetActiveProfile();
    assert(!getResult.has_value());
    assert(getResult.error() == Client::Core::CommandError::Disconnected);
}

int main() {
    test_load_and_active_profile();
    test_invalid_directory();
    test_set_invalid_profile();
    test_get_without_active();
    
    std::cout << "All ServerProfileRegistry tests passed successfully.\n";
    return 0;
}
