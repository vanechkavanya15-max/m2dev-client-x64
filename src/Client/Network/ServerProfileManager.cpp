#include "ServerProfileManager.h"
#include <fstream>
#include <sstream>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

namespace Client::Network {

Client::Core::Result<void, Client::Core::CommandError> ServerProfileManager::LoadProfile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return LoadProfileFromJson(buffer.str());
}

Client::Core::Result<void, Client::Core::CommandError> ServerProfileManager::LoadProfileFromJson(const std::string& jsonContent) {
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(jsonContent.c_str());
    
    if (!ok) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    if (!doc.IsObject() || !doc.HasMember("name") || !doc["name"].IsString()) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    ServerProfile profile;
    profile.name = doc["name"].GetString();

    if (!doc.HasMember("auth") || !doc["auth"].IsObject()) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    const auto& authNode = doc["auth"];
    if (!authNode.HasMember("ip") || !authNode["ip"].IsString() ||
        !authNode.HasMember("port") || !authNode["port"].IsUint()) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    profile.auth.ip = authNode["ip"].GetString();
    profile.auth.port = static_cast<uint16_t>(authNode["port"].GetUint());

    if (doc.HasMember("channels") && doc["channels"].IsArray()) {
        const auto& channelsArray = doc["channels"].GetArray();
        for (rapidjson::SizeType i = 0; i < channelsArray.Size(); i++) {
            const auto& channelNode = channelsArray[i];
            if (!channelNode.IsObject() ||
                !channelNode.HasMember("ip") || !channelNode["ip"].IsString() ||
                !channelNode.HasMember("port") || !channelNode["port"].IsUint()) {
                return std::unexpected(Client::Core::CommandError::InvalidParameter);
            }

            ServerProfile::Address channelAddr;
            channelAddr.ip = channelNode["ip"].GetString();
            channelAddr.port = static_cast<uint16_t>(channelNode["port"].GetUint());
            profile.channels.push_back(channelAddr);
        }
    }

    profiles_[profile.name] = profile;
    return {};
}

Client::Core::Result<void, Client::Core::CommandError> ServerProfileManager::SetActiveProfile(const std::string& name) {
    if (profiles_.find(name) == profiles_.end()) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }
    activeProfileName_ = name;
    return {};
}

Client::Core::Result<ServerProfile, Client::Core::CommandError> ServerProfileManager::GetActiveProfile() const {
    if (activeProfileName_.empty()) {
        return std::unexpected(Client::Core::CommandError::Disconnected);
    }
    
    auto it = profiles_.find(activeProfileName_);
    if (it == profiles_.end()) {
        return std::unexpected(Client::Core::CommandError::Disconnected);
    }
    
    return it->second;
}

} // namespace Client::Network
