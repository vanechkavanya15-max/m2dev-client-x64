#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

/**
 * @brief Czysty interfejs dla providerow kryptografii sieciowej (Sodium, Blowfish CTR, AES itp.).
 */
class INetCryptoProvider
{
public:
    virtual ~INetCryptoProvider() = default;

    /**
     * @brief Szyfruje bufor danych in-place (strumieniowo dla pakietow wychodzacych TX).
     * @param buffer Wskaznik do modyfikowanego bufora danych.
     * @param len Liczba bajtow do zaszyfrowania.
     */
    virtual void EncryptInPlace(void* buffer, size_t len) = 0;

    /**
     * @brief Deszyfruje bufor danych in-place (strumieniowo dla pakietow przychodzacych RX).
     * @param buffer Wskaznik do modyfikowanego bufora danych.
     * @param len Liczba bajtow do odszyfrowania.
     */
    virtual void DecryptInPlace(void* buffer, size_t len) = 0;

    /**
     * @brief Zwraca true, jesli szyfrowanie/deszyfrowanie jest aktywne.
     */
    virtual bool IsActivated() const noexcept = 0;

    /**
     * @brief Aktywuje lub dezaktywuje szyfrowanie sieciowe.
     */
    virtual void SetActivated(bool value) noexcept = 0;

    /**
     * @brief Czysci klucze, stany wewnetrzne i zeruje bufory pamieci.
     */
    virtual void CleanUp() noexcept = 0;

    /**
     * @brief Zwraca czytelna nazwe providera (np. "Sodium", "Blowfish").
     */
    virtual std::string_view GetProviderName() const noexcept = 0;
};
