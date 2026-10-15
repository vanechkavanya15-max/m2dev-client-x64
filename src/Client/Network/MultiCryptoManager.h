#pragma once

#include "ServerProfile.h"
#include "ICryptoProvider.h"
#include "../../EterBase/Result.h"
#include <memory>
#include <cstdint>
#include <string_view>
#include <span>

namespace Client::Network {

class MultiCryptoManager {
public:
    explicit MultiCryptoManager(const ServerProfile& profile);
    ~MultiCryptoManager() = default;

    MultiCryptoManager(const MultiCryptoManager&) = delete;
    MultiCryptoManager& operator=(const MultiCryptoManager&) = delete;
    MultiCryptoManager(MultiCryptoManager&&) noexcept = default;
    MultiCryptoManager& operator=(MultiCryptoManager&&) noexcept = default;

    void ActivateEncryption();
    [[nodiscard]] bool IsEncryptionActive() const noexcept;

    [[nodiscard]] EterBase::VoidResult<std::string_view> ProcessStream(uint8_t* data, size_t length);
    [[nodiscard]] EterBase::VoidResult<std::string_view> SetSessionKey(std::span<const uint8_t> key);

private:
    std::unique_ptr<ICryptoProvider> m_provider;
    bool m_isActive{false};
};

} // namespace Client::Network
