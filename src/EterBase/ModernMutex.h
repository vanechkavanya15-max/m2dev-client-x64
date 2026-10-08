#pragma once

#include <mutex>
#include <shared_mutex>
#include <utility>

/**
 * @file ModernMutex.h
 * @brief Nowoczesne prymitywy synchronizacji watkow i RAII w standardzie C++23 dla EterBase.
 * 
 * Zastepuje przestarzale i podatne na bledy Win32 CRITICAL_SECTION
 * w pelni bezpiecznymi dla wyjatkow mechanizmami RAII:
 * - ModernMutex (opakowanie std::mutex z interfejsem C++23 i metodami pomocniczymi)
 * - ModernSharedMutex (wspolbiezny odczyt wielu watkow / wylaczny zapis jednego watku)
 * - ModernRecursiveMutex (wielokrotne blokowanie w ramach tego samego watku)
 * - ScopedLock (szablon RAII std::scoped_lock z automatycznym zapobieganiem zakleszczeniom)
 * - SharedLock (szablon RAII std::shared_lock do wspolbieznego odczytu)
 * - UniqueLock (szablon RAII std::unique_lock z pelna kontrola cyklu zycia zamka)
 */

namespace EterBase {

/**
 * @class ModernMutex
 * @brief Nowoczesny zamiennik dla Win32 CRITICAL_SECTION oparty na std::mutex.
 * 
 * Klasa spelnia koncept C++ BasicLockable oraz Lockable, dzieki czemu
 * bezposrednio wspolpracuje ze standardowymi straznikami RAII (std::scoped_lock, std::unique_lock).
 * Dostarcza takze zgodny interfejs pomocniczy (Lock, Unlock, TryLock, Trylock).
 */
class ModernMutex {
public:
    constexpr ModernMutex() noexcept = default;
    ~ModernMutex() = default;

    // Muteksy nie moga byc kopiowane ani przenoszone
    ModernMutex(const ModernMutex&) = delete;
    ModernMutex& operator=(const ModernMutex&) = delete;
    ModernMutex(ModernMutex&&) = delete;
    ModernMutex& operator=(ModernMutex&&) = delete;

    /**
     * @brief Blokuje muteks na wylacznosc. Watek oczekuje do momentu zwolnienia.
     */
    void lock() {
        m_mutex.lock();
    }

    /**
     * @brief Zwalnia blokade muteksu.
     */
    void unlock() noexcept {
        m_mutex.unlock();
    }

    /**
     * @brief Probuje zajac blokade bez wstrzymywania watku.
     * @return true jesli zablokowano pomyslnie, false w przeciwnym razie.
     */
    [[nodiscard]] bool try_lock() noexcept {
        return m_mutex.try_lock();
    }

    // Metin2 PascalCase helper interface (zgodnosc wsteczna z kodem klienta)
    void Lock() {
        lock();
    }

    void Unlock() noexcept {
        unlock();
    }

    [[nodiscard]] bool TryLock() noexcept {
        return try_lock();
    }

    [[nodiscard]] bool Trylock() noexcept {
        return try_lock();
    }

    /**
     * @brief Dostep do bazowego obiektu std::mutex.
     */
    [[nodiscard]] std::mutex& GetUnderlying() noexcept {
        return m_mutex;
    }

    [[nodiscard]] const std::mutex& GetUnderlying() const noexcept {
        return m_mutex;
    }

private:
    std::mutex m_mutex;
};

/**
 * @class ModernSharedMutex
 * @brief Muteks typu czytelnicy-pisarz (Reader-Writer) oparty na std::shared_mutex.
 * 
 * Pozwala wielu watkom czytajacym dzielic blokade jednoczesnie (SharedLock / lock_shared),
 * gwarantujac jednoczesnie wylacznosc watkowi modyfikujacemu (ScopedLock / lock).
 */
class ModernSharedMutex {
public:
    ModernSharedMutex() = default;
    ~ModernSharedMutex() = default;

    ModernSharedMutex(const ModernSharedMutex&) = delete;
    ModernSharedMutex& operator=(const ModernSharedMutex&) = delete;
    ModernSharedMutex(ModernSharedMutex&&) = delete;
    ModernSharedMutex& operator=(ModernSharedMutex&&) = delete;

    // --- Blokada wylaczna (Write / Exclusive) ---

    void lock() {
        m_mutex.lock();
    }

    void unlock() noexcept {
        m_mutex.unlock();
    }

    [[nodiscard]] bool try_lock() noexcept {
        return m_mutex.try_lock();
    }

    void Lock() {
        lock();
    }

    void Unlock() noexcept {
        unlock();
    }

    [[nodiscard]] bool TryLock() noexcept {
        return try_lock();
    }

    [[nodiscard]] bool Trylock() noexcept {
        return try_lock();
    }

    // --- Blokada wspoldzielona (Read / Shared) ---

    void lock_shared() {
        m_mutex.lock_shared();
    }

    void unlock_shared() noexcept {
        m_mutex.unlock_shared();
    }

    [[nodiscard]] bool try_lock_shared() noexcept {
        return m_mutex.try_lock_shared();
    }

    void LockShared() {
        lock_shared();
    }

    void UnlockShared() noexcept {
        unlock_shared();
    }

    [[nodiscard]] bool TryLockShared() noexcept {
        return try_lock_shared();
    }

    /**
     * @brief Dostep do bazowego obiektu std::shared_mutex.
     */
    [[nodiscard]] std::shared_mutex& GetUnderlying() noexcept {
        return m_mutex;
    }

    [[nodiscard]] const std::shared_mutex& GetUnderlying() const noexcept {
        return m_mutex;
    }

private:
    std::shared_mutex m_mutex;
};

/**
 * @class ModernRecursiveMutex
 * @brief Rekurencyjny muteks oparty na std::recursive_mutex.
 * 
 * Pozwala na wielokrotne zablokowanie przez ten sam watek bez ryzyka zakleszczenia.
 */
class ModernRecursiveMutex {
public:
    ModernRecursiveMutex() = default;
    ~ModernRecursiveMutex() = default;

    ModernRecursiveMutex(const ModernRecursiveMutex&) = delete;
    ModernRecursiveMutex& operator=(const ModernRecursiveMutex&) = delete;
    ModernRecursiveMutex(ModernRecursiveMutex&&) = delete;
    ModernRecursiveMutex& operator=(ModernRecursiveMutex&&) = delete;

    void lock() {
        m_mutex.lock();
    }

    void unlock() noexcept {
        m_mutex.unlock();
    }

    [[nodiscard]] bool try_lock() noexcept {
        return m_mutex.try_lock();
    }

    void Lock() {
        lock();
    }

    void Unlock() noexcept {
        unlock();
    }

    [[nodiscard]] bool TryLock() noexcept {
        return try_lock();
    }

    [[nodiscard]] bool Trylock() noexcept {
        return try_lock();
    }

    [[nodiscard]] std::recursive_mutex& GetUnderlying() noexcept {
        return m_mutex;
    }

    [[nodiscard]] const std::recursive_mutex& GetUnderlying() const noexcept {
        return m_mutex;
    }

private:
    std::recursive_mutex m_mutex;
};

// --- Szablony straznikow RAII w standardzie C++23 ---

/**
 * @brief Szablon RAII ScopedLock oparty na std::scoped_lock.
 * Automatycznie blokuje jeden lub wiele muteksow w konstruktorze,
 * a nastepnie zwalnia je w destruktorze przy wyjsciu ze scope (zarowno normalnym, jak i przez wyjatek).
 * W przypadku wielu muteksow stosuje algorytm deadlock-avoidance.
 */
template <typename... MutexTypes>
using ScopedLock = std::scoped_lock<MutexTypes...>;

/**
 * @brief Szablon RAII SharedLock oparty na std::shared_lock.
 * Przeznaczony do wspolbieznego bezpiecznego odczytu wielu watkow z ModernSharedMutex.
 */
template <typename MutexType = ModernSharedMutex>
using SharedLock = std::shared_lock<MutexType>;

/**
 * @brief Szablon RAII UniqueLock oparty na std::unique_lock.
 * Zapewnia elastyczne zarzadzanie czasem zycia blokady wylacznej.
 */
template <typename MutexType = ModernMutex>
using UniqueLock = std::unique_lock<MutexType>;

/**
 * @brief Szablon RAII LockGuard oparty na std::lock_guard.
 */
template <typename MutexType = ModernMutex>
using LockGuard = std::lock_guard<MutexType>;

} // namespace EterBase

// Globalne aliasy dla wygody i bezposredniej kompatybilnosci w calym silniku
using EterBase::ModernMutex;
using EterBase::ModernSharedMutex;
using EterBase::ModernRecursiveMutex;
using EterBase::ScopedLock;
using EterBase::SharedLock;
using EterBase::UniqueLock;
using EterBase::LockGuard;
