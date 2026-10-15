#pragma once

#include "NetCryptoProvider.h"
#include <span>
#include <array>
#include <mutex>
#include <atomic>

/**
 * @brief BlowfishCryptoProvider - implementacja INetCryptoProvider w trybie Blowfish CTR dla profilu Luria.
 * Obsluguje:
 *  - 64-bitowy rozmiar bloku (8 bajtow).
 *  - Inkrementacje licznika big-endian.
 *  - Osobne klucze i wektory IV dla strumieni TX i RX.
 *  - In-place encrypt / decrypt strumieniowy (CTR keystream buffer).
 *  - Bezpieczenstwo watkowe (niezalezne muteksy TX i RX) oraz zero wyciekow pamieci.
 */
class BlowfishCryptoProvider final : public INetCryptoProvider
{
public:
    static constexpr size_t BLOCK_SIZE = 8;
    static constexpr size_t MIN_KEY_SIZE = 4;
    static constexpr size_t MAX_KEY_SIZE = 56;

    BlowfishCryptoProvider();
    ~BlowfishCryptoProvider() override;

    // INetCryptoProvider interface
    void EncryptInPlace(void* buffer, size_t len) override;
    void DecryptInPlace(void* buffer, size_t len) override;
    bool IsActivated() const noexcept override;
    void SetActivated(bool value) noexcept override;
    void CleanUp() noexcept override;
    std::string_view GetProviderName() const noexcept override;

    /**
     * @brief Ustawia osobne klucze i wektory IV dla kanalow TX i RX (profil Luria / DH2 handshake).
     * @param txKey Klucz szyfrowania wychodzacego TX (od 4 do 56 bajtow).
     * @param txIv Poczatkowy wektor IV licznika TX (8 bajtow).
     * @param rxKey Klucz deszyfrowania przychodzacego RX (od 4 do 56 bajtow).
     * @param rxIv Poczatkowy wektor IV licznika RX (8 bajtow).
     */
    void SetKeys(std::span<const uint8_t> txKey,
                 std::span<const uint8_t> txIv,
                 std::span<const uint8_t> rxKey,
                 std::span<const uint8_t> rxIv);

private:
    struct ChannelState
    {
        std::array<uint32_t, 18> p{};
        std::array<std::array<uint32_t, 256>, 4> s{};
        std::array<uint8_t, BLOCK_SIZE> counter{};
        std::array<uint8_t, BLOCK_SIZE> keystream{};
        size_t blockOffset = 0;
        bool initialized = false;

        void Reset() noexcept;
    };

    static void BlowfishEncryptBlock(uint32_t& xl, uint32_t& xr,
                                     const std::array<uint32_t, 18>& p,
                                     const std::array<std::array<uint32_t, 256>, 4>& s) noexcept;

    static void IncrementCounter(std::array<uint8_t, BLOCK_SIZE>& counter) noexcept;
    static void GenerateKeystreamBlock(ChannelState& channel) noexcept;
    static void InitChannel(ChannelState& channel, std::span<const uint8_t> key, std::span<const uint8_t> iv);
    static void ProcessChannelStream(ChannelState& channel, void* buffer, size_t len) noexcept;

    ChannelState m_txChannel;
    ChannelState m_rxChannel;

    mutable std::mutex m_txMutex;
    mutable std::mutex m_rxMutex;
    std::atomic<bool> m_activated{false};
};
