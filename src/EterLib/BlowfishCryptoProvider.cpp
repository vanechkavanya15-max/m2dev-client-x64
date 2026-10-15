#include "StdAfx.h"
#include "BlowfishCryptoProvider.h"
#include "BlowfishConstants.inl"
#include <sodium.h>
#include <algorithm>
#include <cstring>

namespace {

inline uint32_t BlowfishF(uint32_t x, const std::array<std::array<uint32_t, 256>, 4>& s) noexcept
{
    const uint8_t a = static_cast<uint8_t>(x >> 24);
    const uint8_t b = static_cast<uint8_t>(x >> 16);
    const uint8_t c = static_cast<uint8_t>(x >> 8);
    const uint8_t d = static_cast<uint8_t>(x);
    return ((s[0][a] + s[1][b]) ^ s[2][c]) + s[3][d];
}

} // namespace

void BlowfishCryptoProvider::ChannelState::Reset() noexcept
{
    sodium_memzero(p.data(), sizeof(p));
    sodium_memzero(s.data(), sizeof(s));
    sodium_memzero(counter.data(), sizeof(counter));
    sodium_memzero(keystream.data(), sizeof(keystream));
    blockOffset = 0;
    initialized = false;
}

BlowfishCryptoProvider::BlowfishCryptoProvider()
{
}

BlowfishCryptoProvider::~BlowfishCryptoProvider()
{
    CleanUp();
}

void BlowfishCryptoProvider::BlowfishEncryptBlock(uint32_t& xl, uint32_t& xr,
                                                  const std::array<uint32_t, 18>& p,
                                                  const std::array<std::array<uint32_t, 256>, 4>& s) noexcept
{
    for (int i = 0; i < 16; ++i)
    {
        xl ^= p[i];
        xr ^= BlowfishF(xl, s);
        std::swap(xl, xr);
    }
    std::swap(xl, xr);
    xr ^= p[16];
    xl ^= p[17];
}

void BlowfishCryptoProvider::IncrementCounter(std::array<uint8_t, BLOCK_SIZE>& counter) noexcept
{
    for (int i = static_cast<int>(BLOCK_SIZE) - 1; i >= 0; --i)
    {
        if (++counter[i] != 0)
            break;
    }
}

void BlowfishCryptoProvider::GenerateKeystreamBlock(ChannelState& channel) noexcept
{
    uint32_t xl = (static_cast<uint32_t>(channel.counter[0]) << 24) |
                  (static_cast<uint32_t>(channel.counter[1]) << 16) |
                  (static_cast<uint32_t>(channel.counter[2]) << 8) |
                  static_cast<uint32_t>(channel.counter[3]);
    uint32_t xr = (static_cast<uint32_t>(channel.counter[4]) << 24) |
                  (static_cast<uint32_t>(channel.counter[5]) << 16) |
                  (static_cast<uint32_t>(channel.counter[6]) << 8) |
                  static_cast<uint32_t>(channel.counter[7]);

    BlowfishEncryptBlock(xl, xr, channel.p, channel.s);

    channel.keystream[0] = static_cast<uint8_t>(xl >> 24);
    channel.keystream[1] = static_cast<uint8_t>(xl >> 16);
    channel.keystream[2] = static_cast<uint8_t>(xl >> 8);
    channel.keystream[3] = static_cast<uint8_t>(xl);
    channel.keystream[4] = static_cast<uint8_t>(xr >> 24);
    channel.keystream[5] = static_cast<uint8_t>(xr >> 16);
    channel.keystream[6] = static_cast<uint8_t>(xr >> 8);
    channel.keystream[7] = static_cast<uint8_t>(xr);
}

void BlowfishCryptoProvider::InitChannel(ChannelState& channel,
                                         std::span<const uint8_t> key,
                                         std::span<const uint8_t> iv)
{
    channel.p = BLOWFISH_ORIG_P;
    channel.s = BLOWFISH_ORIG_S;

    if (!key.empty())
    {
        size_t keyPos = 0;
        for (int i = 0; i < 18; ++i)
        {
            uint32_t data = 0;
            for (int k = 0; k < 4; ++k)
            {
                data = (data << 8) | key[keyPos];
                keyPos = (keyPos + 1) % key.size();
            }
            channel.p[i] ^= data;
        }

        uint32_t datal = 0;
        uint32_t datar = 0;
        for (int i = 0; i < 18; i += 2)
        {
            BlowfishEncryptBlock(datal, datar, channel.p, channel.s);
            channel.p[i] = datal;
            channel.p[i + 1] = datar;
        }

        for (int box = 0; box < 4; ++box)
        {
            for (int i = 0; i < 256; i += 2)
            {
                BlowfishEncryptBlock(datal, datar, channel.p, channel.s);
                channel.s[box][i] = datal;
                channel.s[box][i + 1] = datar;
            }
        }
    }

    // Skopiuj wektor IV (maks. 8 bajtow)
    sodium_memzero(channel.counter.data(), channel.counter.size());
    const size_t copyIvLen = (std::min)(iv.size(), BLOCK_SIZE);
    if (copyIvLen > 0)
    {
        std::memcpy(channel.counter.data(), iv.data(), copyIvLen);
    }

    // Wygeneruj pierwszy blok keystreamu z poczatkowego wektora IV
    GenerateKeystreamBlock(channel);
    channel.blockOffset = 0;
    channel.initialized = true;
}

void BlowfishCryptoProvider::ProcessChannelStream(ChannelState& channel, void* buffer, size_t len) noexcept
{
    if (!channel.initialized || !buffer || len == 0)
        return;

    uint8_t* bytePtr = static_cast<uint8_t*>(buffer);
    for (size_t i = 0; i < len; ++i)
    {
        if (channel.blockOffset >= BLOCK_SIZE)
        {
            IncrementCounter(channel.counter);
            GenerateKeystreamBlock(channel);
            channel.blockOffset = 0;
        }

        bytePtr[i] ^= channel.keystream[channel.blockOffset++];
    }
}

void BlowfishCryptoProvider::SetKeys(std::span<const uint8_t> txKey,
                                     std::span<const uint8_t> txIv,
                                     std::span<const uint8_t> rxKey,
                                     std::span<const uint8_t> rxIv)
{
    std::lock_guard lockTx(m_txMutex);
    std::lock_guard lockRx(m_rxMutex);

    m_txChannel.Reset();
    m_rxChannel.Reset();

    InitChannel(m_txChannel, txKey, txIv);
    InitChannel(m_rxChannel, rxKey, rxIv);
}

void BlowfishCryptoProvider::EncryptInPlace(void* buffer, size_t len)
{
    if (!m_activated.load(std::memory_order_acquire) || !buffer || len == 0)
        return;

    std::lock_guard lock(m_txMutex);
    ProcessChannelStream(m_txChannel, buffer, len);
}

void BlowfishCryptoProvider::DecryptInPlace(void* buffer, size_t len)
{
    if (!m_activated.load(std::memory_order_acquire) || !buffer || len == 0)
        return;

    std::lock_guard lock(m_rxMutex);
    ProcessChannelStream(m_rxChannel, buffer, len);
}

bool BlowfishCryptoProvider::IsActivated() const noexcept
{
    return m_activated.load(std::memory_order_acquire);
}

void BlowfishCryptoProvider::SetActivated(bool value) noexcept
{
    m_activated.store(value, std::memory_order_release);
}

void BlowfishCryptoProvider::CleanUp() noexcept
{
    std::lock_guard lockTx(m_txMutex);
    std::lock_guard lockRx(m_rxMutex);
    m_activated.store(false, std::memory_order_release);
    m_txChannel.Reset();
    m_rxChannel.Reset();
}

std::string_view BlowfishCryptoProvider::GetProviderName() const noexcept
{
    return "Blowfish";
}
