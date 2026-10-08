#include "../../EterBase/StdAfx.h"
#pragma once

#include <span>
#include <cstdint>
#include <memory>

namespace Client::Network {

    enum class CryptoType {
        None,
        AES,
        XTEA
    };

    struct ServerProfile {
        // Dummy fields for the profile
        uint32_t serverId{0};
        uint32_t channelId{0};
    };

    class ICryptoProvider {
    public:
        virtual ~ICryptoProvider() = default;

        virtual void Encrypt(std::span<uint8_t> buffer) = 0;
        virtual void Decrypt(std::span<uint8_t> buffer) = 0;
        virtual bool IsActive() const noexcept = 0;
    };

    std::unique_ptr<ICryptoProvider> CreateProvider(CryptoType type, const ServerProfile& profile);

} // namespace Client::Network
