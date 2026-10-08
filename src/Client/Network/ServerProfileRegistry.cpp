#include "ServerProfileRegistry.h"
#include <fstream>
#include <sstream>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

namespace Client::Network {

Client::Core::Result<void, Client::Core::CommandError> ServerProfileRegistry::LoadAllFromDirectory(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                continue; // Skip file if we can't open it
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string jsonContent = buffer.str();

            rapidjson::Document doc;
            rapidjson::ParseResult ok = doc.Parse(jsonContent.c_str());
            
            if (!ok) {
                return std::unexpected(Client::Core::CommandError::InvalidParameter);
            }

            if (!doc.IsObject() || !doc.HasMember("name") || !doc["name"].IsString()) {
                return std::unexpected(Client::Core::CommandError::InvalidParameter);
            }

            ServerProfile profile;
            profile.SetProfileName(doc["name"].GetString());

            if (doc.HasMember("host") && doc["host"].IsString()) {
                profile.SetHost(doc["host"].GetString());
            }

            if (doc.HasMember("port") && doc["port"].IsUint()) {
                profile.SetPort(static_cast<uint16_t>(doc["port"].GetUint()));
            }

            if (doc.HasMember("framing") && doc["framing"].IsString()) {
                std::string framingStr = doc["framing"].GetString();
                if (framingStr == "Ymir1B") {
                    profile.SetFramingMode(FramingMode::Ymir1B);
                } else if (framingStr == "M2Dev4B") {
                    profile.SetFramingMode(FramingMode::M2Dev4B);
                } else {
                    return std::unexpected(Client::Core::CommandError::InvalidParameter);
                }
            }

            if (doc.HasMember("crypto") && doc["crypto"].IsString()) {
                std::string cryptoStr = doc["crypto"].GetString();
                if (cryptoStr == "AES") {
                    profile.SetCryptoType(CryptoType::AES);
                } else if (cryptoStr == "TEA") {
                    profile.SetCryptoType(CryptoType::TEA);
                } else if (cryptoStr == "None") {
                    profile.SetCryptoType(CryptoType::Plain);
                } else {
                    return std::unexpected(Client::Core::CommandError::InvalidParameter);
                }
            }

            if (doc.HasMember("opcodes") && doc["opcodes"].IsObject()) {
                const auto& opcodes = doc["opcodes"].GetObject();
                for (auto itr = opcodes.MemberBegin(); itr != opcodes.MemberEnd(); ++itr) {
                    if (itr->value.IsUint()) {
                        profile.RegisterOpcode(itr->name.GetString(), static_cast<uint8_t>(itr->value.GetUint()));
                    }
                }
            }

            profiles_[profile.GetProfileName()] = profile;
        }
    }

    return {};
}

Client::Core::Result<void, Client::Core::CommandError> ServerProfileRegistry::SetActiveProfile(const std::string& name) {
    if (profiles_.find(name) == profiles_.end()) {
        return std::unexpected(Client::Core::CommandError::InvalidParameter);
    }
    activeProfileName_ = name;
    return {};
}

Client::Core::Result<ServerProfile, Client::Core::CommandError> ServerProfileRegistry::GetActiveProfile() const {
    if (activeProfileName_.empty()) {
        return std::unexpected(Client::Core::CommandError::Disconnected);
    }
    
    auto it = profiles_.find(activeProfileName_);
    if (it == profiles_.end()) {
        return std::unexpected(Client::Core::CommandError::Disconnected); // Better error code?
    }
    
    return it->second;
}

} // namespace Client::Network
