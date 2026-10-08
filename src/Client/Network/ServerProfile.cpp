#include "../../StdAfx.h"
#include "ServerProfile.h"

namespace Client::Network {

void ServerProfile::SetHost(const std::string& host) {
    m_host = host;
}

void ServerProfile::SetPort(uint16_t port) {
    m_port = port;
}

void ServerProfile::SetProfileName(const std::string& name) {
    m_profileName = name;
}

[[nodiscard]] EterBase::VoidResult<> ServerProfile::SetHeaderSizeBytes(uint8_t size) {
    if (size != 1 && size != 2) {
        return EterBase::MakeError("Header size must be 1 or 2 bytes");
    }
    m_headerSizeBytes = size;
    return {};
}

void ServerProfile::SetFramingMode(FramingMode mode) {
    m_framingMode = mode;
}

void ServerProfile::SetCryptoType(CryptoType type) {
    m_cryptoType = type;
}

void ServerProfile::RegisterOpcode(const std::string& logicName, uint8_t opcode) {
    m_opcodes[logicName] = opcode;
}

void ServerProfile::SetMoveInterval(uint32_t ms) {
    m_moveIntervalMs = ms;
}

void ServerProfile::SetPickupInterval(uint32_t ms) {
    m_pickupIntervalMs = ms;
}

void ServerProfile::SetAttackCrcRequired(bool required) {
    m_attackCrcRequired = required;
}

[[nodiscard]] const std::string& ServerProfile::GetHost() const {
    return m_host;
}

[[nodiscard]] uint16_t ServerProfile::GetPort() const {
    return m_port;
}

[[nodiscard]] const std::string& ServerProfile::GetProfileName() const {
    return m_profileName;
}

[[nodiscard]] uint8_t ServerProfile::GetHeaderSizeBytes() const {
    return m_headerSizeBytes;
}

[[nodiscard]] FramingMode ServerProfile::GetFramingMode() const {
    return m_framingMode;
}

[[nodiscard]] CryptoType ServerProfile::GetCryptoType() const {
    return m_cryptoType;
}

[[nodiscard]] EterBase::Result<uint8_t> ServerProfile::GetOpcode(const std::string& logicName) const {
    auto it = m_opcodes.find(logicName);
    if (it == m_opcodes.end()) {
        return EterBase::MakeError("Opcode not found");
    }
    return it->second;
}

[[nodiscard]] uint32_t ServerProfile::GetMoveInterval() const {
    return m_moveIntervalMs;
}

[[nodiscard]] uint32_t ServerProfile::GetPickupInterval() const {
    return m_pickupIntervalMs;
}

[[nodiscard]] bool ServerProfile::IsAttackCrcRequired() const {
    return m_attackCrcRequired;
}

} // namespace Client::Network
