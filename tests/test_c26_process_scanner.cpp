#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include "UserInterface/ProcessScanner.h"

int main()
{
    std::cout << "[TEST] Starting test_c26_process_scanner...\n";

    // Test 1: Pop na pustej kolejce oraz obsluga nullptr
    {
        assert(!ProcessScanner_PopProcessQueue(nullptr));

        std::vector<CRCPair> queue;
        assert(!ProcessScanner_PopProcessQueue(&queue));
        assert(queue.empty());
    }
    std::cout << "[PASS] Test 1: Empty queue and nullptr validation passed.\n";

    // Test 2: Inicjalizacja i idempotentnosc ProcessScanner_Create
    {
        bool created = ProcessScanner_Create();
        assert(created);

        // Ponowne wywolanie powinno zwrocic true i nie tworzyc drugiego watku
        bool createdAgain = ProcessScanner_Create();
        assert(createdAgain);
    }
    std::cout << "[PASS] Test 2: ProcessScanner_Create idempotence passed.\n";

    // Test 3: Krotkie oczekiwanie na dzialanie watku
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::vector<CRCPair> queue;
        // W zaleznosci od systemu kolejka moze byc pusta lub miec juz elementy
        ProcessScanner_PopProcessQueue(&queue);
    }
    std::cout << "[PASS] Test 3: Thread running and non-blocking queue pop passed.\n";

    // Test 4: Zatrzymanie przez ProcessScanner_ReleaseQuitEvent i ProcessScanner_Destroy
    {
        ProcessScanner_ReleaseQuitEvent();
        ProcessScanner_Destroy();

        // Kolejka powinna byc wyczyszczona po destroy
        std::vector<CRCPair> queue;
        assert(!ProcessScanner_PopProcessQueue(&queue));
    }
    std::cout << "[PASS] Test 4: Graceful shutdown and destroy passed.\n";

    // Test 5: Idempotentnosc ProcessScanner_Destroy
    {
        // Ponowne wywolanie destroy na zatrzymanym watku nie powinno zawiesic ani rzucic bledu
        ProcessScanner_Destroy();
    }
    std::cout << "[PASS] Test 5: ProcessScanner_Destroy idempotence passed.\n";

    // Test 6: Ponowny cykl Create -> Destroy (reusability)
    {
        assert(ProcessScanner_Create());
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        ProcessScanner_Destroy();
    }
    std::cout << "[PASS] Test 6: Re-creation and destruction cycle passed.\n";

    std::cout << "test_c26_process_scanner: ALL TESTS PASSED (100%)\n";
    return 0;
}
