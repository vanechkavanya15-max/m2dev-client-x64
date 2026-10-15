#include <cassert>
#include <iostream>
#include "../src/Client/Network/ServerProfileManager.h"
#include "../src/Client/Core/DomainCommands.h"
#include <fstream>
#include <filesystem>

void test_valid_json() {
    Client::Network::ServerProfileManager manager;
    
    std::string validJson = R"({
        "name": "Pandora",
        "auth": {
            "ip": "127.0.0.1",
            "port": 50000
        },
        "channels": [
            {
                "ip": "127.0.0.1",
                "port": 50010
            },
            {
                "ip": "127.0.0.1",
                "port": 50020
            }
        ]
    })";

    auto result = manager.LoadProfileFromJson(validJson);
    assert(result.has_value());

    auto setActiveResult = manager.SetActiveProfile("Pandora");
    assert(setActiveResult.has_value());

    auto profileResult = manager.GetActiveProfile();
    assert(profileResult.has_value());
    
    const auto& profile = profileResult.value();
    assert(profile.name == "Pandora");
    assert(profile.auth.ip == "127.0.0.1");
    assert(profile.auth.port == 50000);
    assert(profile.channels.size() == 2);
    assert(profile.channels[0].port == 50010);
    assert(profile.channels[1].port == 50020);
}

void test_invalid_json() {
    Client::Network::ServerProfileManager manager;
    
    // Missing auth
    std::string invalidJson1 = R"({
        "name": "Invalid"
    })";

    auto result1 = manager.LoadProfileFromJson(invalidJson1);
    assert(!result1.has_value());
    assert(result1.error() == Client::Core::CommandError::InvalidParameter);

    // Bad JSON syntax
    std::string invalidJson2 = R"({ "name": "Broken", })";
    auto result2 = manager.LoadProfileFromJson(invalidJson2);
    assert(!result2.has_value());
}

void test_no_active_profile() {
    Client::Network::ServerProfileManager manager;
    
    auto profileResult = manager.GetActiveProfile();
    assert(!profileResult.has_value());
    assert(profileResult.error() == Client::Core::CommandError::Disconnected);
}

void test_load_from_file() {
    std::string filePath = "temp_test_profile.json";
    std::ofstream out(filePath);
    out << R"({
        "name": "Classic",
        "auth": { "ip": "192.168.1.1", "port": 1234 },
        "channels": []
    })";
    out.close();

    Client::Network::ServerProfileManager manager;
    auto result = manager.LoadProfile(filePath);
    assert(result.has_value());

    auto setActiveResult = manager.SetActiveProfile("Classic");
    assert(setActiveResult.has_value());

    auto profileResult = manager.GetActiveProfile();
    assert(profileResult.has_value());
    assert(profileResult.value().name == "Classic");

    std::filesystem::remove(filePath);
}

int main() {
    test_valid_json();
    test_invalid_json();
    test_no_active_profile();
    test_load_from_file();

    std::cout << "All ServerProfileManager tests passed successfully.\n";
    return 0;
}
