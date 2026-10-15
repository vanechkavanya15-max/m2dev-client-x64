#include "../src/EterBase/ModernMutex.h"

#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <stdexcept>

using namespace EterBase;

#define ASSERT_TEST(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Blad asercji: " << (msg) << " w linii " << __LINE__ << "\n"; \
            assert(cond); \
        } \
    } while (false)

// 1. Podstawowe blokowanie i zwalnianie oraz metody pomocnicze
void TestBasicLockUnlock() {
    ModernMutex mtx;

    // Standardowe metody
    mtx.lock();
    mtx.unlock();

    bool acquired = mtx.try_lock();
    ASSERT_TEST(acquired, "try_lock powinine zwrocic true dla wolnego muteksu");
    mtx.unlock();

    // Metody pomocnicze PascalCase dla wstecznej zgodnosci
    mtx.Lock();
    mtx.Unlock();

    acquired = mtx.TryLock();
    ASSERT_TEST(acquired, "TryLock powinien zwrocic true");
    mtx.Unlock();

    acquired = mtx.Trylock();
    ASSERT_TEST(acquired, "Trylock powinien zwrocic true");
    mtx.Unlock();

    // Weryfikacja blokowania przez inny watek
    mtx.lock();
    std::atomic<bool> threadSawLocked{false};
    std::thread t([&]() {
        if (!mtx.try_lock()) {
            threadSawLocked = true;
        } else {
            mtx.unlock();
        }
    });
    t.join();
    ASSERT_TEST(threadSawLocked.load(), "Inny watek nie powinien moc zajac zablokowanego muteksu");
    mtx.unlock();

    std::cout << "[TEST] TestBasicLockUnlock: Sukces\n";
}

// 2. Bezpieczenstwo RAII przy uzyciu ScopedLock
void TestScopedLockRAII() {
    ModernMutex mtx;
    std::atomic<bool> insideScopeTested{false};

    {
        ScopedLock lock(mtx);

        // W obrebie zasiegu inny watek nie moze zajac muteksu
        std::thread t([&]() {
            if (!mtx.try_lock()) {
                insideScopeTested = true;
            } else {
                mtx.unlock();
            }
        });
        t.join();
        ASSERT_TEST(insideScopeTested.load(), "Muteks powinine byc zablokowany wewnatrz zasiegu RAII");
    }

    // Po wyjsciu ze scope muteks musi byc natychmiast zwolniony
    bool lockAfterScope = mtx.try_lock();
    ASSERT_TEST(lockAfterScope, "Muteks powinine byc zwolniony automatycznie po wyjsciu ze scope");
    mtx.unlock();

    std::cout << "[TEST] TestScopedLockRAII: Sukces\n";
}

// 3. Bezpieczenstwo RAII przy wystapieniu wyjatku (brak deadlocka)
void TestScopedLockExceptionSafety() {
    ModernMutex mtx;
    bool exceptionCaught = false;

    try {
        ScopedLock lock(mtx);
        throw std::runtime_error("Testowy wyjatek wewnatrz krytycznej sekcji");
    } catch (const std::exception&) {
        exceptionCaught = true;
    }

    ASSERT_TEST(exceptionCaught, "Wyjatek powinien zostac zlapany");

    // Muteks musi byc zwolniony pomimo zgloszenia wyjatku
    bool acquired = mtx.try_lock();
    ASSERT_TEST(acquired, "Muteks musi byc zwolniony po zgloszeniu wyjatku (RAII exception safety)");
    mtx.unlock();

    std::cout << "[TEST] TestScopedLockExceptionSafety: Sukces\n";
}

// 4. Wspolbiezny dostep wielu watkow bez wyscigow (Data Race Freedom)
void TestConcurrentContention() {
    ModernMutex mtx;
    int sharedCounter = 0;
    constexpr int threadCount = 8;
    constexpr int iterations = 10000;

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < iterations; ++j) {
                ScopedLock lock(mtx);
                ++sharedCounter;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_TEST(sharedCounter == threadCount * iterations, "Wspolbiezny licznik musi miec dokladna wartosc");
    std::cout << "[TEST] TestConcurrentContention: Sukces (" << sharedCounter << " inkrementacji)\n";
}

// 5. Test ModernSharedMutex - wspolbiezny odczyt (SharedLock) i wylaczny zapis (ScopedLock)
void TestModernSharedMutex() {
    ModernSharedMutex sharedMtx;
    std::atomic<int> concurrentReaders{0};
    std::atomic<bool> bothReading{false};

    // Test wspolbieznego odczytu
    {
        SharedLock reader1(sharedMtx);
        concurrentReaders++;

        std::thread t2([&]() {
            SharedLock reader2(sharedMtx);
            concurrentReaders++;
            if (concurrentReaders.load() == 2) {
                bothReading = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            concurrentReaders--;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        t2.join();
        concurrentReaders--;
    }

    ASSERT_TEST(bothReading.load(), "Dwa watki czytajace powinny dzialac jednoczesnie z SharedLock");

    // Test wylacznego zapisu blokujacego odczyt
    {
        ScopedLock writer(sharedMtx);

        std::atomic<bool> readBlocked{false};
        std::thread readerThread([&]() {
            if (!sharedMtx.try_lock_shared()) {
                readBlocked = true;
            } else {
                sharedMtx.unlock_shared();
            }
        });
        readerThread.join();
        ASSERT_TEST(readBlocked.load(), "Watek czytajacy musi byc zablokowany gdy pisarz trzyma zamek wylaczny");
    }

    // Sprawdzenie metod pomocniczych PascalCase
    sharedMtx.LockShared();
    ASSERT_TEST(!sharedMtx.TryLock(), "TryLock wylaczny musi zwrocic false podczas aktywnego LockShared");
    sharedMtx.UnlockShared();

    ASSERT_TEST(sharedMtx.TryLockShared(), "TryLockShared powinien zwrocic true gdy zamek jest wolny");
    sharedMtx.UnlockShared();

    std::cout << "[TEST] TestModernSharedMutex: Sukces\n";
}

// 6. Test zapobiegania zakleszczeniom (Deadlock Avoidance) przy wielu muteksach
void TestDeadlockAvoidanceMultiLock() {
    ModernMutex mtxA;
    ModernMutex mtxB;
    constexpr int iterations = 1000;
    std::atomic<bool> successA{false};
    std::atomic<bool> successB{false};

    // Watek 1 blokuje (mtxA, mtxB)
    std::thread t1([&]() {
        for (int i = 0; i < iterations; ++i) {
            ScopedLock lock(mtxA, mtxB);
            // Krotka operacja w sekcji krytycznej
        }
        successA = true;
    });

    // Watek 2 blokuje w odwrotnej kolejnosci (mtxB, mtxA)
    std::thread t2([&]() {
        for (int i = 0; i < iterations; ++i) {
            ScopedLock lock(mtxB, mtxA);
            // Krotka operacja w sekcji krytycznej
        }
        successB = true;
    });

    t1.join();
    t2.join();

    ASSERT_TEST(successA.load() && successB.load(), "Brak zakleszczen (deadlock-free) przy blokowaniu wielu muteksow");
    std::cout << "[TEST] TestDeadlockAvoidanceMultiLock: Sukces\n";
}

// 7. Test ModernRecursiveMutex
void TestModernRecursiveMutex() {
    ModernRecursiveMutex recMtx;

    // Ten sam watek moze wielokrotnie zablokowac zamek rekurencyjny
    recMtx.Lock();
    recMtx.Lock();
    recMtx.Unlock();
    recMtx.Unlock();

    {
        ScopedLock lock1(recMtx);
        {
            ScopedLock lock2(recMtx);
            // Zagniezdzone sekcje RAII
        }
    }

    ASSERT_TEST(recMtx.TryLock(), "Po wyjsciu ze wszystkich zagniezdzen zamek rekurencyjny musi byc wolny");
    recMtx.Unlock();

    std::cout << "[TEST] TestModernRecursiveMutex: Sukces\n";
}

// 8. Test elastycznosci UniqueLock
void TestUniqueLockFlexibility() {
    ModernMutex mtx;
    {
        UniqueLock lock(mtx);
        ASSERT_TEST(lock.owns_lock(), "UniqueLock musi posiadac blokade");

        lock.unlock();
        ASSERT_TEST(!lock.owns_lock(), "UniqueLock po recznym unlock nie posiada blokady");

        // Inny watek moze teraz przejac zamek
        bool otherAcquired = false;
        std::thread t([&]() {
            if (mtx.try_lock()) {
                otherAcquired = true;
                mtx.unlock();
            }
        });
        t.join();
        ASSERT_TEST(otherAcquired, "Inny watek powinien moc zablokowac po manualnym unlock");

        lock.lock();
        ASSERT_TEST(lock.owns_lock(), "UniqueLock ponownie posiada blokade");
    }

    ASSERT_TEST(mtx.try_lock(), "Muteks musi byc zwolniony po zniszczeniu UniqueLock");
    mtx.unlock();

    std::cout << "[TEST] TestUniqueLockFlexibility: Sukces\n";
}

int main() {
    std::cout << "=== Uruchomienie testow jednostkowych ModernMutex C++23 ===\n";

    TestBasicLockUnlock();
    TestScopedLockRAII();
    TestScopedLockExceptionSafety();
    TestConcurrentContention();
    TestModernSharedMutex();
    TestDeadlockAvoidanceMultiLock();
    TestModernRecursiveMutex();
    TestUniqueLockFlexibility();

    std::cout << "=== Wszystkie testy ModernMutex zakonczone sukcesem! ===\n";
    return 0;
}
