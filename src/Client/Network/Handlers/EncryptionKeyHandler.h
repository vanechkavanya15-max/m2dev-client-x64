#pragma once

#include <span>
#include <cstdint>
#include <optional>
#include <vector>
#include <algorithm>
#include "../../../EterBase/Result.h"

namespace Client::Network {

/**
 * @brief Handler odpowiedzialny za inicjalizacje i rotacje kluczy szyfrowania strumienia (TEA/Sodium).
 * 
 * Klasa bezpieczna dla pamieci, korzystajaca z C++20 std::span i std::expected.
 */
class EncryptionKeyHandler {
public:
    static constexpr size_t REQUIRED_KEY_SIZE = 32; // Przykladowy wymagany rozmiar dla Sodium

    constexpr EncryptionKeyHandler() noexcept = default;
    ~EncryptionKeyHandler() = default;

    // Zabronienie kopiowania
    EncryptionKeyHandler(const EncryptionKeyHandler&) = delete;
    EncryptionKeyHandler& operator=(const EncryptionKeyHandler&) = delete;

    // Przenoszenie dozwolone
    constexpr EncryptionKeyHandler(EncryptionKeyHandler&&) noexcept = default;
    constexpr EncryptionKeyHandler& operator=(EncryptionKeyHandler&&) noexcept = default;

    /**
     * @brief Inicjalizuje klucz glowny szyfrowania.
     * 
     * @param key Bufor zawierajacy klucz. Musi miec odpowiedni rozmiar.
     * @return Sukces (void) lub PacketError w przypadku bledu (np. zly rozmiar).
     */
    [[nodiscard]] constexpr EterBase::PacketResult<void> Initialize(std::span<const uint8_t> key) {
        if (key.size() != REQUIRED_KEY_SIZE) {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        m_currentKey.emplace(key.begin(), key.end());
        m_initialized = true;

        return {};
    }

    /**
     * @brief Rotuje klucz na nowy.
     * 
     * Wymaga wczesniejszej inicjalizacji. 
     * @param newKey Bufor zawierajacy nowy klucz.
     * @return Sukces (void) lub PacketError w przypadku bledu.
     */
    [[nodiscard]] constexpr EterBase::PacketResult<void> RotateKey(std::span<const uint8_t> newKey) {
        if (!m_initialized) {
            return EterBase::MakeError(EterBase::PacketError::SessionClosed);
        }

        if (newKey.size() != REQUIRED_KEY_SIZE) {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        m_currentKey.emplace(newKey.begin(), newKey.end());
        
        return {};
    }

    /**
     * @brief Sprawdza czy handler zostal zainicjalizowany poprawnym kluczem.
     */
    [[nodiscard]] constexpr bool IsInitialized() const noexcept {
        return m_initialized;
    }

    /**
     * @brief Pobiera aktualny klucz w formie bezpiecznego widoku, jesli istnieje.
     */
    [[nodiscard]] constexpr std::optional<std::span<const uint8_t>> GetCurrentKey() const noexcept {
        if (m_initialized && m_currentKey.has_value()) {
            return std::span<const uint8_t>(*m_currentKey);
        }
        return std::nullopt;
    }

private:
    bool m_initialized = false;
    std::optional<std::vector<uint8_t>> m_currentKey;
};

} // namespace Client::Network
