#include "StdAfx.h"
#include "SodiumCryptoProvider.h"

SodiumCryptoProvider::SodiumCryptoProvider()
{
}

SodiumCryptoProvider::~SodiumCryptoProvider()
{
    CleanUp();
}

void SodiumCryptoProvider::EncryptInPlace(void* buffer, size_t len)
{
    if (!m_cipher.IsActivated() || !buffer || len == 0)
        return;

    std::lock_guard lock(m_txMutex);
    m_cipher.EncryptInPlace(buffer, len);
}

void SodiumCryptoProvider::DecryptInPlace(void* buffer, size_t len)
{
    if (!m_cipher.IsActivated() || !buffer || len == 0)
        return;

    std::lock_guard lock(m_rxMutex);
    m_cipher.DecryptInPlace(buffer, len);
}

bool SodiumCryptoProvider::IsActivated() const noexcept
{
    return m_cipher.IsActivated();
}

void SodiumCryptoProvider::SetActivated(bool value) noexcept
{
    std::lock_guard lockTx(m_txMutex);
    std::lock_guard lockRx(m_rxMutex);
    m_cipher.SetActivated(value);
}

void SodiumCryptoProvider::CleanUp() noexcept
{
    std::lock_guard lockTx(m_txMutex);
    std::lock_guard lockRx(m_rxMutex);
    m_cipher.CleanUp();
}

std::string_view SodiumCryptoProvider::GetProviderName() const noexcept
{
    return "Sodium";
}
