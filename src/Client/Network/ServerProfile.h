#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include "../../EterBase/Result.h"

namespace Client::Network {

enum class FramingMode : uint8_t {
    Ymir1B = 0,
    M2Dev4B
};

enum class CryptoType : uint8_t {
    Plain = 0,
    TEA,
    AES,
    Sodium
};

class ServerProfile {
public:
    ServerProfile() = default;
    ~ServerProfile() = default;

    ServerProfile(const ServerProfile&) = default;
    ServerProfile& operator=(const ServerProfile&) = default;
    ServerProfile(ServerProfile&&) noexcept = default;
    ServerProfile& operator=(ServerProfile&&) noexcept = default;

    // Setters
    void SetHost(const std::string& host);
    void SetPort(uint16_t port);
    void SetProfileName(const std::string& name);

    [[nodiscard]] EterBase::VoidResult<> SetHeaderSizeBytes(uint8_t size);
    void SetFramingMode(FramingMode mode);
    void SetCryptoType(CryptoType type);

    void RegisterOpcode(const std::string& logicName, uint8_t opcode);
    void SetMoveInterval(uint32_t ms);
    void SetPickupInterval(uint32_t ms);
    void SetAttackCrcRequired(bool required);

    // Getters
    [[nodiscard]] const std::string& GetHost() const;
    [[nodiscard]] uint16_t GetPort() const;
    [[nodiscard]] const std::string& GetProfileName() const;

    [[nodiscard]] uint8_t GetHeaderSizeBytes() const;
    [[nodiscard]] FramingMode GetFramingMode() const;
    [[nodiscard]] CryptoType GetCryptoType() const;

    [[nodiscard]] EterBase::Result<uint8_t> GetOpcode(const std::string& logicName) const;
    
    [[nodiscard]] uint32_t GetMoveInterval() const;
    [[nodiscard]] uint32_t GetPickupInterval() const;
    [[nodiscard]] bool IsAttackCrcRequired() const;

private:
    std::string m_host;
    std::string m_profileName;
    uint16_t m_port{0};
    
    uint8_t m_headerSizeBytes{1};
    FramingMode m_framingMode{FramingMode::Ymir1B};
    CryptoType m_cryptoType{CryptoType::Plain};

    std::unordered_map<std::string, uint8_t> m_opcodes;

    uint32_t m_moveIntervalMs{0};
    uint32_t m_pickupIntervalMs{0};
    bool m_attackCrcRequired{false};
};

} // namespace Client::Network
