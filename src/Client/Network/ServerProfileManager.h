#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <cstdint>
#include "Client/Core/DomainCommands.h"
#include "Client/Core/Result.h"

namespace Client::Network {

struct ServerProfile {
    std::string name;
    
    struct Address {
        std::string ip;
        uint16_t port;
    };
    
    Address auth;
    std::vector<Address> channels;
};

class ServerProfileManager {
public:
    ServerProfileManager() = default;
    ~ServerProfileManager() = default;

    ServerProfileManager(const ServerProfileManager&) = delete;
    ServerProfileManager& operator=(const ServerProfileManager&) = delete;

    // Loads a server profile from a JSON string or file path
    Client::Core::Result<void, Client::Core::CommandError> LoadProfile(const std::filesystem::path& path);
    Client::Core::Result<void, Client::Core::CommandError> LoadProfileFromJson(const std::string& jsonContent);

    // Sets the active profile by name
    Client::Core::Result<void, Client::Core::CommandError> SetActiveProfile(const std::string& name);

    // Retrieves the currently active profile
    Client::Core::Result<ServerProfile, Client::Core::CommandError> GetActiveProfile() const;

private:
    std::unordered_map<std::string, ServerProfile> profiles_;
    std::string activeProfileName_;
};

} // namespace Client::Network
