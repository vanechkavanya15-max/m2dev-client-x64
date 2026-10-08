#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include "ServerProfile.h"
#include "Client/Core/DomainCommands.h"
#include "Client/Core/Result.h"

namespace Client::Network {

class ServerProfileRegistry {
public:
    ServerProfileRegistry() = default;
    ~ServerProfileRegistry() = default;

    ServerProfileRegistry(const ServerProfileRegistry&) = delete;
    ServerProfileRegistry& operator=(const ServerProfileRegistry&) = delete;

    Client::Core::Result<void, Client::Core::CommandError> LoadAllFromDirectory(const std::filesystem::path& path);
    Client::Core::Result<void, Client::Core::CommandError> SetActiveProfile(const std::string& name);
    Client::Core::Result<ServerProfile, Client::Core::CommandError> GetActiveProfile() const;

private:
    std::unordered_map<std::string, ServerProfile> profiles_;
    std::string activeProfileName_;
};

} // namespace Client::Network
