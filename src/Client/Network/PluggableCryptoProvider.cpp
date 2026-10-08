#include "../../EterBase/StdAfx.h"
#include "PluggableCryptoProvider.h"

namespace Client::Network {

    class NullCryptoProvider final : public ICryptoProvider {
    public:
        void Encrypt(std::span<uint8_t> buffer) override {
            // No encryption
        }

        void Decrypt(std::span<uint8_t> buffer) override {
            // No decryption
        }

        bool IsActive() const noexcept override {
            return false;
        }
    };

    class AesCryptoProvider final : public ICryptoProvider {
    public:
        void Encrypt(std::span<uint8_t> buffer) override {
            // Dummy logic for AES: XOR with 0xAA
            for (auto& b : buffer) {
                b ^= 0xAA;
            }
        }

        void Decrypt(std::span<uint8_t> buffer) override {
            // Dummy logic for AES: XOR with 0xAA
            for (auto& b : buffer) {
                b ^= 0xAA;
            }
        }

        bool IsActive() const noexcept override {
            return true;
        }
    };

    class XteaCryptoProvider final : public ICryptoProvider {
    public:
        void Encrypt(std::span<uint8_t> buffer) override {
            // Dummy logic for XTEA: bitwise NOT
            for (auto& b : buffer) {
                b = ~b;
            }
        }

        void Decrypt(std::span<uint8_t> buffer) override {
            // Dummy logic for XTEA: bitwise NOT
            for (auto& b : buffer) {
                b = ~b;
            }
        }

        bool IsActive() const noexcept override {
            return true;
        }
    };

    std::unique_ptr<ICryptoProvider> CreateProvider(CryptoType type, const ServerProfile& profile) {
        switch (type) {
            case CryptoType::AES:
                return std::make_unique<AesCryptoProvider>();
            case CryptoType::XTEA:
                return std::make_unique<XteaCryptoProvider>();
            case CryptoType::None:
            default:
                return std::make_unique<NullCryptoProvider>();
        }
    }

} // namespace Client::Network
