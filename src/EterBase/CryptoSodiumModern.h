#pragma once

#include <sodium.h>
#include <cstdint>
#include <cstring>
#include <span>
#include <optional>
#include <vector>
#include <array>

#include "Result.h"
#include "StrongTypes.h"
#include "LogModern.h"
#include "../UserInterface/Core/EventBus.h"

namespace EterBase {

/**
 * @brief Event triggered when the cryptographic session state changes.
 */
struct CryptoStateChangedEvent : public UserInterface::Core::IEvent {
    bool isActivated;

    /**
     * @brief Constructs the event.
     * @param activated The new activation state.
     */
    explicit CryptoStateChangedEvent(bool activated) : isActivated(activated) {}
};

/**
 * @brief Modern C++23 wrapper for libsodium cryptographic operations.
 * 
 * Replaces the old Crypto++ implementation. Uses std::span for safe buffer access,
 * and std::expected for robust error handling. Decoupled from UI via EventBus.
 */
class CryptoSodiumModern {
public:
    /**
     * @brief Constructs the CryptoSodiumModern instance.
     */
    CryptoSodiumModern() = default;

    /**
     * @brief Destructs the CryptoSodiumModern instance, cleaning up sensitive data.
     */
    ~CryptoSodiumModern() {
        CleanUp();
    }

    /**
     * @brief Initializes the cryptographic context (generates keypair).
     * @return PacketResult<void> indicating success or failure.
     */
    PacketResult<void> Initialize() {
        if (sodium_init() < 0) {
            ModernLogger::Log(LogLevel::Error, "libsodium initialization failed.");
            return MakeError(PacketError::MalformedPayload);
        }
        
        crypto_kx_keypair(publicKey_.data(), secretKey_.data());
        isInitialized_ = true;
        
        ModernLogger::Log(LogLevel::Info, "CryptoSodiumModern initialized successfully.");
        return {};
    }

    /**
     * @brief Cleans up sensitive cryptographic data.
     */
    void CleanUp() {
        sodium_memzero(secretKey_.data(), secretKey_.size());
        sodium_memzero(rxKey_.data(), rxKey_.size());
        sodium_memzero(txKey_.data(), txKey_.size());
        isInitialized_ = false;
        
        SetActivated(false);
    }

    /**
     * @brief Gets the public key if initialized.
     * @return An optional span representing the public key.
     */
    std::optional<std::span<const uint8_t>> GetPublicKey() const {
        return std::optional<bool>(isInitialized_)
            .and_then([this](bool init) -> std::optional<std::span<const uint8_t>> {
                if (init) return std::span<const uint8_t>(publicKey_);
                return std::nullopt;
            });
    }

    /**
     * @brief Computes client session keys from the server's public key.
     * @param serverKey The server's public key.
     * @return PacketResult<void> on success or error.
     */
    PacketResult<void> ComputeClientKeys(std::span<const uint8_t> serverKey) {
        if (!isInitialized_) {
            return MakeError(PacketError::SessionClosed);
        }
        if (serverKey.size() != crypto_kx_PUBLICKEYBYTES) {
            return MakeError(PacketError::MalformedPayload);
        }

        if (crypto_kx_client_session_keys(
                rxKey_.data(), txKey_.data(),
                publicKey_.data(), secretKey_.data(),
                serverKey.data()) != 0) {
            ModernLogger::Log(LogLevel::Error, "Failed to compute client session keys.");
            return MakeError(PacketError::MalformedPayload);
        }

        SetActivated(true);
        return {};
    }

    /**
     * @brief Encrypts the provided buffer in-place.
     * @param buffer The buffer to encrypt.
     * @return PacketResult<void> on success.
     */
    PacketResult<void> EncryptInPlace(std::span<uint8_t> buffer) {
        if (!isActivated_) {
            return MakeError(PacketError::SessionClosed);
        }

        std::array<uint8_t, crypto_stream_xchacha20_NONCEBYTES> nonce{};
        std::memcpy(nonce.data(), &txNonce_, sizeof(txNonce_));

        crypto_stream_xchacha20_xor(
            buffer.data(), buffer.data(), buffer.size(),
            nonce.data(), txKey_.data());

        txNonce_++;
        return {};
    }

    /**
     * @brief Decrypts the provided buffer in-place.
     * @param buffer The buffer to decrypt.
     * @return PacketResult<void> on success.
     */
    PacketResult<void> DecryptInPlace(std::span<uint8_t> buffer) {
        if (!isActivated_) {
            return MakeError(PacketError::SessionClosed);
        }

        std::array<uint8_t, crypto_stream_xchacha20_NONCEBYTES> nonce{};
        std::memcpy(nonce.data(), &rxNonce_, sizeof(rxNonce_));

        crypto_stream_xchacha20_xor(
            buffer.data(), buffer.data(), buffer.size(),
            nonce.data(), rxKey_.data());

        rxNonce_++;
        return {};
    }

    /**
     * @brief Encrypts an item vnum.
     * @param vnum The item vnum to encrypt.
     * @return Optional containing the encrypted bytes.
     */
    std::optional<std::vector<uint8_t>> EncryptItemVnum(ItemVnum vnum) {
        return std::optional<ItemVnum>(vnum)
            .and_then([this](ItemVnum v) -> std::optional<std::vector<uint8_t>> {
                uint32_t val = v.value();
                std::vector<uint8_t> buf(sizeof(val));
                std::memcpy(buf.data(), &val, sizeof(val));
                auto res = EncryptInPlace(buf);
                if (!res) {
                    return std::nullopt;
                }
                return buf;
            });
    }

    /**
     * @brief Decrypts an entity id.
     * @param buffer The buffer containing the encrypted entity id.
     * @return Optional containing the decrypted entity id.
     */
    std::optional<EntityId> DecryptEntityId(std::span<uint8_t> buffer) {
        return std::optional<std::span<uint8_t>>(buffer)
            .and_then([this](std::span<uint8_t> buf) -> std::optional<EntityId> {
                if (buf.size() != sizeof(uint32_t)) return std::nullopt;
                if (!DecryptInPlace(buf)) return std::nullopt;
                
                uint32_t val = 0;
                std::memcpy(&val, buf.data(), sizeof(val));
                return EntityId(val);
            });
    }

    /**
     * @brief Checks if the cryptographic session is activated.
     * @return True if activated, false otherwise.
     */
    bool IsActivated() const {
        return isActivated_;
    }

private:
    /**
     * @brief Sets the activation state and triggers an event.
     * @param activated The new state.
     */
    void SetActivated(bool activated) {
        if (isActivated_ != activated) {
            isActivated_ = activated;
            UserInterface::Core::EventBus::GetInstance().Publish(CryptoStateChangedEvent(activated));
            ModernLogger::Log(LogLevel::Info, "Crypto session state changed to: {}", activated);
        }
    }

    bool isInitialized_ = false;
    bool isActivated_ = false;
    uint64_t txNonce_ = 0;
    uint64_t rxNonce_ = 0;

    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKey_{};
    std::array<uint8_t, crypto_kx_SECRETKEYBYTES> secretKey_{};
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> rxKey_{};
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> txKey_{};
};

} // namespace EterBase
