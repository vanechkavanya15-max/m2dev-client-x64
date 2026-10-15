#undef NDEBUG
#include <cassert>
#include <iostream>
#include <vector>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <cstdlib>

#include "Graphics/DynamicGeometryRingBuffer.h"

#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "[BLAD ASERCJI] Warunek nie spelniony: (" #condition ") w pliku " \
                      << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (0)

using namespace Client::Graphics;

/**
 * @brief Test 1: Liniowe alokacje – sprawdzanie kolejnych offsetow, baseVertex i flagi NOOVERWRITE.
 */
static void Test1_LinearAllocations()
{
    std::cout << "[TEST] 1. Liniowe alokacje wierzcholkow (NOOVERWRITE, offsety, baseVertex)..." << std::endl;

    DynamicGeometryRingBuffer ring;
    const bool initOk = ring.Initialize(nullptr);
    TEST_ASSERT(initOk);
    TEST_ASSERT(ring.IsInitialized());
    TEST_ASSERT(ring.IsEmulationMode());
    TEST_ASSERT(ring.GetVertexCapacity() == DynamicGeometryRingBuffer::kDefaultVertexCapacity);
    TEST_ASSERT(ring.GetIndexCapacity() == DynamicGeometryRingBuffer::kDefaultIndexCapacity);

    constexpr uint32_t stride = 32; // 32 bajty per wierzcholek

    // 1. Pierwsza alokacja: 100 wierzcholkow (3200 bajtow)
    auto alloc1 = ring.AllocateVertices(100, stride);
    TEST_ASSERT(alloc1.pData != nullptr);
    TEST_ASSERT(alloc1.offsetBytes == 0);
    TEST_ASSERT(alloc1.baseVertex == 0);
    TEST_ASSERT(!alloc1.isDiscardWrap); // NOOVERWRITE
    TEST_ASSERT(ring.GetVertexOffset() == 3200);

    // 2. Druga alokacja: 200 wierzcholkow (6400 bajtow)
    auto alloc2 = ring.AllocateVertices(200, stride);
    TEST_ASSERT(alloc2.pData != nullptr);
    TEST_ASSERT(alloc2.offsetBytes == 3200);
    TEST_ASSERT(alloc2.baseVertex == 100); // 3200 / 32 = 100
    TEST_ASSERT(!alloc2.isDiscardWrap); // NOOVERWRITE
    TEST_ASSERT(ring.GetVertexOffset() == 9600);

    // 3. Trzecia alokacja: 300 wierzcholkow (9600 bajtow)
    auto alloc3 = ring.AllocateVertices(300, stride);
    TEST_ASSERT(alloc3.pData != nullptr);
    TEST_ASSERT(alloc3.offsetBytes == 9600);
    TEST_ASSERT(alloc3.baseVertex == 300); // 9600 / 32 = 300
    TEST_ASSERT(!alloc3.isDiscardWrap); // NOOVERWRITE
    TEST_ASSERT(ring.GetVertexOffset() == 19200);

    // Sprawdzenie telemetrii
    TEST_ASSERT(ring.GetNoOverwriteHits() == 3);
    TEST_ASSERT(ring.GetDiscardWraps() == 0);
    TEST_ASSERT(ring.GetTotalBytesStreamed() == 19200);
    TEST_ASSERT(ring.GetStallFreeRatio() == 100.0);

    std::cout << "       [PASS] Kolejne offsety [0, 3200, 9600] oraz baseVertex [0, 100, 300] zgodne z NOOVERWRITE.\n" << std::endl;
}

/**
 * @brief Test 2: Przekroczenie pojemnosci (Wrap-Around) – weryfikacja czy nastepuje poprawny DISCARD i powrot do offsetu 0.
 */
static void Test2_WrapAroundDiscard()
{
    std::cout << "[TEST] 2. Przekroczenie pojemnosci (Wrap-Around, flaga DISCARD i powrot do offsetu 0)..." << std::endl;

    // Testujemy na buforze o mniejszym kontrolowanym rozmiarze: 64 KB wierzcholki, 16 KB indeksy
    constexpr uint32_t testVertexCap = 64 * 1024; // 65 536 bajtow
    constexpr uint32_t testIndexCap  = 16 * 1024; // 16 384 bajty

    DynamicGeometryRingBuffer ring(testVertexCap, testIndexCap);
    const bool initOk = ring.Initialize(nullptr);
    TEST_ASSERT(initOk);

    constexpr uint32_t stride = 32;

    // Krok 1: Wypelnij bufor do 48 000 bajtow (1500 wierzcholkow * 32)
    auto alloc1 = ring.AllocateVertices(1500, stride);
    TEST_ASSERT(alloc1.pData != nullptr);
    TEST_ASSERT(alloc1.offsetBytes == 0);
    TEST_ASSERT(alloc1.baseVertex == 0);
    TEST_ASSERT(!alloc1.isDiscardWrap); // Miesci sie w buforze
    TEST_ASSERT(ring.GetVertexOffset() == 48000);
    TEST_ASSERT(ring.GetNoOverwriteHits() == 1);
    TEST_ASSERT(ring.GetDiscardWraps() == 0);

    // Krok 2: Zaalokuj 1000 wierzcholkow (32 000 bajtow).
    // 48 000 + 32 000 = 80 000 > 65 536 (pojemnosc).
    // Wymusza Wrap-Around: powrot do offsetu 0, flaga DISCARD!
    auto alloc2 = ring.AllocateVertices(1000, stride);
    TEST_ASSERT(alloc2.pData != nullptr);
    TEST_ASSERT(alloc2.offsetBytes == 0); // Powrot do poczatku pierscienia
    TEST_ASSERT(alloc2.baseVertex == 0);
    TEST_ASSERT(alloc2.isDiscardWrap);    // Wymuszony DISCARD dla GPU
    TEST_ASSERT(ring.GetVertexOffset() == 32000);
    TEST_ASSERT(ring.GetNoOverwriteHits() == 1);
    TEST_ASSERT(ring.GetDiscardWraps() == 1);

    // Krok 3: Kolejna alokacja po wrap-around: 500 wierzcholkow (16 000 bajtow).
    // 32 000 + 16 000 = 48 000 <= 65 536. Powrot do trybu NOOVERWRITE!
    auto alloc3 = ring.AllocateVertices(500, stride);
    TEST_ASSERT(alloc3.pData != nullptr);
    TEST_ASSERT(alloc3.offsetBytes == 32000);
    TEST_ASSERT(alloc3.baseVertex == 1000); // 32000 / 32 = 1000
    TEST_ASSERT(!alloc3.isDiscardWrap);     // NOOVERWRITE
    TEST_ASSERT(ring.GetVertexOffset() == 48000);
    TEST_ASSERT(ring.GetNoOverwriteHits() == 2);
    TEST_ASSERT(ring.GetDiscardWraps() == 1);

    // Weryfikacja wrap-around dla bufora indeksow
    // Krok 4: Alokacja 7000 indeksow (14 000 bajtow < 16 384)
    auto idx1 = ring.AllocateIndices(7000);
    TEST_ASSERT(idx1.pData != nullptr);
    TEST_ASSERT(idx1.offsetBytes == 0);
    TEST_ASSERT(idx1.startIndex == 0);
    TEST_ASSERT(!idx1.isDiscardWrap);
    TEST_ASSERT(ring.GetIndexOffset() == 14000);

    // Krok 5: Alokacja 2000 indeksow (4000 bajtow).
    // 14 000 + 4000 = 18 000 > 16 384. Wymusza Wrap-Around bufora indeksow!
    auto idx2 = ring.AllocateIndices(2000);
    TEST_ASSERT(idx2.pData != nullptr);
    TEST_ASSERT(idx2.offsetBytes == 0); // Powrot do offsetu 0
    TEST_ASSERT(idx2.startIndex == 0);
    TEST_ASSERT(idx2.isDiscardWrap);    // DISCARD
    TEST_ASSERT(ring.GetIndexOffset() == 4000);
    TEST_ASSERT(ring.GetDiscardWraps() == 2);

    std::cout << "       [PASS] Wrap-Around dziala bezblednie: powrot do 0, flaga DISCARD ustawiona, poprawny powrot do NOOVERWRITE.\n" << std::endl;
}

/**
 * @brief Test 3: Stress Test na 500 000 wierzcholkow w 5 000 paczkach – weryfikacja poprawnosci danych i wskaznika GetStallFreeRatio() > 99%.
 */
static void Test3_StressTest500kVertices()
{
    std::cout << "[TEST] 3. Stress Test na 500 000 wierzcholkow w 5 000 paczkach (weryfikacja danych i StallFreeRatio > 99%)..." << std::endl;

    // Domyslny bufor 8 MB wierzcholkow
    DynamicGeometryRingBuffer ring;
    const bool initOk = ring.Initialize(nullptr);
    TEST_ASSERT(initOk);

    struct Vertex32
    {
        float x, y, z;
        uint32_t color;
        float u, v;
        uint32_t packetId;
        uint32_t vertexIndex;
    };
    static_assert(sizeof(Vertex32) == 32, "Rozmiar wierzcholka musi byc rowny 32 bajty");

    constexpr uint32_t kTotalBatches = 5000;
    constexpr uint32_t kVerticesPerBatch = 100; // 5000 * 100 = 500 000 wierzcholkow
    constexpr uint32_t kStride = sizeof(Vertex32);

    const auto startTime = std::chrono::high_resolution_clock::now();

    for (uint32_t batch = 0; batch < kTotalBatches; ++batch)
    {
        auto alloc = ring.AllocateVertices(kVerticesPerBatch, kStride);
        TEST_ASSERT(alloc.pData != nullptr);

        Vertex32* pVertices = static_cast<Vertex32*>(alloc.pData);

        // Zapis wierzcholkow (symulacja potoku CPU generujacego geometrie dynamiczna)
        for (uint32_t i = 0; i < kVerticesPerBatch; ++i)
        {
            pVertices[i].x = static_cast<float>(batch * 10);
            pVertices[i].y = static_cast<float>(i);
            pVertices[i].z = 0.0f;
            pVertices[i].color = 0xFF00AAFF;
            pVertices[i].u = static_cast<float>(i) / 100.0f;
            pVertices[i].v = 1.0f;
            pVertices[i].packetId = batch;
            pVertices[i].vertexIndex = i;
        }

        // Natychmiastowa weryfikacja spojnosci danych (Data Integrity Verification)
        for (uint32_t i = 0; i < kVerticesPerBatch; ++i)
        {
            TEST_ASSERT(pVertices[i].packetId == batch);
            TEST_ASSERT(pVertices[i].vertexIndex == i);
            TEST_ASSERT(pVertices[i].x == static_cast<float>(batch * 10));
            TEST_ASSERT(pVertices[i].y == static_cast<float>(i));
            TEST_ASSERT(pVertices[i].color == 0xFF00AAFF);
        }

        ring.UnlockVertices();
    }

    const auto endTime = std::chrono::high_resolution_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    const uint64_t noOverwriteHits = ring.GetNoOverwriteHits();
    const uint64_t discardWraps = ring.GetDiscardWraps();
    const uint64_t totalBytes = ring.GetTotalBytesStreamed();
    const double stallFreeRatio = ring.GetStallFreeRatio();

    std::cout << "       Liczba paczek: " << kTotalBatches << ", Laczna liczba wierzcholkow: " << (kTotalBatches * kVerticesPerBatch) << std::endl;
    std::cout << "       Wystrumieniowano bajtow: " << totalBytes << " (" << std::fixed << std::setprecision(2) << (double(totalBytes) / (1024.0 * 1024.0)) << " MB)" << std::endl;
    std::cout << "       Hity NOOVERWRITE: " << noOverwriteHits << ", Wrap-Around DISCARDs: " << discardWraps << std::endl;
    std::cout << "       Stall-Free Ratio: " << std::fixed << std::setprecision(3) << stallFreeRatio << "%" << std::endl;
    std::cout << "       Czas wykonania: " << durationMs << " ms" << std::endl;

    // Weryfikacja wymogow architektonicznych:
    TEST_ASSERT(totalBytes == static_cast<uint64_t>(kTotalBatches) * kVerticesPerBatch * kStride);
    TEST_ASSERT(noOverwriteHits + discardWraps == kTotalBatches);
    TEST_ASSERT(stallFreeRatio > 99.0); // Wymog > 99%
    TEST_ASSERT(discardWraps > 0);      // Potwierdzenie, ze pierscien sie obrocil i zamknal

    std::cout << "       [PASS] Stress Test 500 000 wierzcholkow zaliczony pomyslnie (StallFreeRatio > 99%).\n" << std::endl;
}

/**
 * @brief Test 4: Dynamiczny bufor indeksow (16-bit D3DFMT_INDEX16) – sekwencje i startIndex.
 */
static void Test4_IndexBufferSequences()
{
    std::cout << "[TEST] 4. Dynamiczny bufor indeksow (sekwencje i startIndex)..." << std::endl;

    DynamicGeometryRingBuffer ring;
    const bool initOk = ring.Initialize(nullptr);
    TEST_ASSERT(initOk);

    // Alokacja 1: 300 indeksow (600 bajtow)
    auto idx1 = ring.AllocateIndices(300);
    TEST_ASSERT(idx1.pData != nullptr);
    TEST_ASSERT(idx1.startIndex == 0);
    TEST_ASSERT(idx1.offsetBytes == 0);
    TEST_ASSERT(!idx1.isDiscardWrap);

    uint16_t* pIndices1 = static_cast<uint16_t*>(idx1.pData);
    for (uint16_t i = 0; i < 300; ++i)
    {
        pIndices1[i] = i;
    }
    for (uint16_t i = 0; i < 300; ++i)
    {
        TEST_ASSERT(pIndices1[i] == i);
    }
    ring.UnlockIndices();

    // Alokacja 2: 600 indeksow (1200 bajtow)
    auto idx2 = ring.AllocateIndices(600);
    TEST_ASSERT(idx2.pData != nullptr);
    TEST_ASSERT(idx2.startIndex == 300); // 600 bajtow / 2 = 300
    TEST_ASSERT(idx2.offsetBytes == 600);
    TEST_ASSERT(!idx2.isDiscardWrap);

    uint16_t* pIndices2 = static_cast<uint16_t*>(idx2.pData);
    for (uint16_t i = 0; i < 600; ++i)
    {
        pIndices2[i] = static_cast<uint16_t>(1000 + i);
    }
    for (uint16_t i = 0; i < 600; ++i)
    {
        TEST_ASSERT(pIndices2[i] == static_cast<uint16_t>(1000 + i));
    }
    ring.UnlockIndices();

    std::cout << "       [PASS] Alokacje indeksow i indeksowanie startowe (startIndex) poprawne.\n" << std::endl;
}

/**
 * @brief Test 5: Przypadki brzegowe, wyrownanie stride i odpornosc na bledne parametry.
 */
static void Test5_AlignmentAndEdgeCases()
{
    std::cout << "[TEST] 5. Przypadki brzegowe i dopasowanie wyrownania stride..." << std::endl;

    DynamicGeometryRingBuffer ring;
    const bool initOk = ring.Initialize(nullptr);
    TEST_ASSERT(initOk);

    // 1. Zerowa liczba wierzcholkow
    auto allocZero = ring.AllocateVertices(0, 32);
    TEST_ASSERT(allocZero.pData == nullptr);

    // 2. Zerowy stride
    auto allocZeroStride = ring.AllocateVertices(100, 0);
    TEST_ASSERT(allocZeroStride.pData == nullptr);

    // 3. Alokacja wieksza niz pojemnosc bufora
    auto allocTooBig = ring.AllocateVertices(DynamicGeometryRingBuffer::kDefaultVertexCapacity / 16, 32);
    TEST_ASSERT(allocTooBig.pData == nullptr);

    // 4. Naprzemienne alokacje o roznych stride (np. stride 28, potem stride 32)
    // Offset musi zostac poprawnie wyrownany do wielokrotnosci nowego stride!
    auto a1 = ring.AllocateVertices(3, 28); // 84 bajty (niepodzielne przez 32)
    TEST_ASSERT(a1.pData != nullptr);
    TEST_ASSERT(a1.offsetBytes == 0);
    TEST_ASSERT(ring.GetVertexOffset() == 84);

    auto a2 = ring.AllocateVertices(2, 32); // Nowy stride = 32. 84 zaokragla w gore do 96 (3 * 32).
    TEST_ASSERT(a2.pData != nullptr);
    TEST_ASSERT(a2.offsetBytes == 96);
    TEST_ASSERT(a2.baseVertex == 3); // 96 / 32 = 3
    TEST_ASSERT(ring.GetVertexOffset() == 96 + (2 * 32));

    // 5. ResetFrame i ResetTelemetry
    ring.ResetFrame();
    TEST_ASSERT(ring.GetNoOverwriteHits() > 0);
    ring.ResetTelemetry();
    TEST_ASSERT(ring.GetNoOverwriteHits() == 0);
    TEST_ASSERT(ring.GetDiscardWraps() == 0);
    TEST_ASSERT(ring.GetTotalBytesStreamed() == 0);
    TEST_ASSERT(ring.GetStallFreeRatio() == 100.0);

    // 6. Release i ponowna inicjalizacja
    ring.Release();
    TEST_ASSERT(!ring.IsInitialized());
    auto allocAfterRelease = ring.AllocateVertices(10, 32);
    TEST_ASSERT(allocAfterRelease.pData == nullptr);

    TEST_ASSERT(ring.Initialize(nullptr));
    TEST_ASSERT(ring.IsInitialized());
    auto allocReinit = ring.AllocateVertices(10, 32);
    TEST_ASSERT(allocReinit.pData != nullptr);

    std::cout << "       [PASS] Przypadki brzegowe i wyrownania stride zaliczone pomyslnie.\n" << std::endl;
}

int main()
{
    std::cout << "==========================================================================" << std::endl;
    std::cout << "=== URUCHAMIANIE TESTOW: Dynamic Geometry Ring Buffer (D3D9 C++23)    ===" << std::endl;
    std::cout << "==========================================================================\n" << std::endl;

    Test1_LinearAllocations();
    Test2_WrapAroundDiscard();
    Test3_StressTest500kVertices();
    Test4_IndexBufferSequences();
    Test5_AlignmentAndEdgeCases();

    std::cout << "==========================================================================" << std::endl;
    std::cout << "=== WSZYSTKIE TESTY DYNAMIC GEOMETRY RING BUFFER ZALICZONE (100% PASS) ===" << std::endl;
    std::cout << "==========================================================================" << std::endl;
    return 0;
}
