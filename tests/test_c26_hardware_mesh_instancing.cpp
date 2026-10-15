#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "Client/Graphics/HardwareMeshInstancer.h"
#include "GameLib/SpeedTreeInstancedBridge.h"

#include <cstdint>
#include <vector>
#include <cmath>
#include <cstring>

using namespace Client::Graphics;
using namespace GameLib;

TEST_CASE("HardwareMeshInstancer - Test 1: Grupowanie 1500 drzew 4 typow w buckety i redukcja DrawCalli")
{
    HardwareMeshInstancer instancer;
    const bool initOk = instancer.Initialize(nullptr);
    CHECK(initOk == true);
    CHECK(instancer.GetRingBuffer().IsInitialized());

    SpeedTreeInstancedBridge bridge;

    // Konfiguracja mapowania 4 typow drzew na unikalne meshId (materialId = 0)
    for (uint32_t type = 0; type < 4; ++type)
    {
        bridge.SetTreeTypeMapping(type, type, 0);
    }

    // Rejestracja 1500 drzew nalezacych cyklicznie do 4 typow
    constexpr uint32_t kTotalTrees = 1500;
    for (uint32_t i = 0; i < kTotalTrees; ++i)
    {
        const uint32_t typeId = i % 4;
        const float posX = static_cast<float>(i * 10);
        const float posY = static_cast<float>(i * 20);
        const float posZ = 0.0f;
        const float scale = 1.0f + (static_cast<float>(i % 5) * 0.1f);
        const float rotationYaw = static_cast<float>(i % 360) * (3.14159265f / 180.0f);
        const uint32_t tintColor = 0xFFFFFFFF;
        const float windPhase = static_cast<float>(i % 100) * 0.1f;
        constexpr uint32_t lodIndex = 0;

        bridge.RegisterTree(typeId, posX, posY, posZ, scale, rotationYaw, tintColor, windPhase, lodIndex);
    }

    CHECK(bridge.GetRegisteredTreeCount() == kTotalTrees);

    // Przekazanie instancji drzew do sprzetowego instancera
    const size_t flushedTrees = bridge.FlushTrees(instancer);
    CHECK(flushedTrees == kTotalTrees);
    CHECK(bridge.GetRegisteredTreeCount() == 0);

    // Weryfikacja: 1500 drzew zostalo zredukowanych do dokladnie 4 bucketow
    CHECK(instancer.GetBucketCount() == 4);
    CHECK(instancer.GetPendingInstanceCount() == kTotalTrees);

    // Sprawdzenie rownomiernego rozkladu w 4 bucketach (1500 / 4 = 375 na bucket)
    for (uint32_t type = 0; type < 4; ++type)
    {
        const auto* pInstances = instancer.GetBucketInstances(BucketKey{.meshId = type, .materialId = 0, .lodIndex = 0});
        REQUIRE(pInstances != nullptr);
        CHECK(pInstances->size() == 375);
    }

    // Wyslanie partii do bufora instancji (symulacja renderowania ramki)
    const uint32_t drawCalls = instancer.Flush(nullptr);

    // Zamiast 1500 pojedynczych DrawCalli wykonano dokladnie 4 DrawCalle
    CHECK(drawCalls == 4);
    CHECK(instancer.GetTotalDrawCalls() == 4);
    CHECK(instancer.GetTotalInstances() == 1500);
    CHECK(instancer.GetBucketCount() == 0);

    // Weryfikacja redukcji DrawCalli: (1 - 4/1500) = 99.733% > 95%
    const double reductionRatio = instancer.GetDrawCallReductionRatio();
    CHECK(reductionRatio > 0.95);
    CHECK(reductionRatio == doctest::Approx(1.0 - (4.0 / 1500.0)));
}

TEST_CASE("HardwareMeshInstancer - Test 2: Poprawnosc danych macierzy instancji w Strumieniu 1")
{
    HardwareMeshInstancer instancer;
    const bool initOk = instancer.Initialize(nullptr);
    CHECK(initOk == true);

    SpeedTreeInstancedBridge bridge;
    bridge.SetRotationAxis(TreeRotationAxis::Z_Up);

    constexpr uint32_t testTreeType = 12;
    constexpr uint32_t testMeshId = 120;
    constexpr uint32_t testMaterialId = 3;
    bridge.SetTreeTypeMapping(testTreeType, testMeshId, testMaterialId);

    const float posX = 125.5f;
    const float posY = 250.25f;
    const float posZ = 500.75f;
    const float scale = 2.0f;
    const float yaw = 0.0f; // Przy yaw=0: cos=1.0, sin=0.0
    const uint32_t tint = 0xFF88CCEE;
    const float wind = 1.75f;
    const uint32_t lod = 2;

    bridge.RegisterTree(testTreeType, posX, posY, posZ, scale, yaw, tint, wind, lod);
    CHECK(bridge.GetRegisteredTreeCount() == 1);

    const size_t flushedCount = bridge.FlushTrees(instancer);
    CHECK(flushedCount == 1);
    CHECK(instancer.GetBucketCount() == 1);

    const BucketKey expectedKey{.meshId = testMeshId, .materialId = testMaterialId, .lodIndex = lod};
    const auto* pBucket = instancer.GetBucketInstances(expectedKey);
    REQUIRE(pBucket != nullptr);
    REQUIRE(pBucket->size() == 1);

    const InstanceData& directData = (*pBucket)[0];

    // Weryfikacja pol macierzy 4x4 (wierszowej row-major)
    // Row 0: X-axis [scale * cos(0), scale * sin(0), 0, 0] = [2, 0, 0, 0]
    CHECK(directData.worldRow0[0] == doctest::Approx(2.0f));
    CHECK(directData.worldRow0[1] == doctest::Approx(0.0f));
    CHECK(directData.worldRow0[2] == doctest::Approx(0.0f));
    CHECK(directData.worldRow0[3] == doctest::Approx(0.0f));

    // Row 1: Y-axis [-scale * sin(0), scale * cos(0), 0, 0] = [0, 2, 0, 0]
    CHECK(directData.worldRow1[0] == doctest::Approx(0.0f));
    CHECK(directData.worldRow1[1] == doctest::Approx(2.0f));
    CHECK(directData.worldRow1[2] == doctest::Approx(0.0f));
    CHECK(directData.worldRow1[3] == doctest::Approx(0.0f));

    // Row 2: Z-axis [0, 0, scale, 0] = [0, 0, 2, 0]
    CHECK(directData.worldRow2[0] == doctest::Approx(0.0f));
    CHECK(directData.worldRow2[1] == doctest::Approx(0.0f));
    CHECK(directData.worldRow2[2] == doctest::Approx(2.0f));
    CHECK(directData.worldRow2[3] == doctest::Approx(0.0f));

    // Row 3: Translation [posX, posY, posZ, 1.0f]
    CHECK(directData.worldRow3[0] == doctest::Approx(posX));
    CHECK(directData.worldRow3[1] == doctest::Approx(posY));
    CHECK(directData.worldRow3[2] == doctest::Approx(posZ));
    CHECK(directData.worldRow3[3] == doctest::Approx(1.0f));

    // Weryfikacja atrybutow instancji
    CHECK(directData.colorTint == tint);
    CHECK(directData.windPhase == doctest::Approx(wind));
    CHECK(directData.lodIndex == lod);
    CHECK(directData.padding == doctest::Approx(0.0f));

    // Przeslanie do bufora strumienia 1
    const uint32_t draws = instancer.Flush(nullptr);
    CHECK(draws == 1);

    // Weryfikacja danych skopiowanych do bufora instancji Strumienia 1
    const auto& flushedInstances = instancer.GetLastFlushedInstanceData();
    REQUIRE(flushedInstances.size() == 1);
    CHECK(flushedInstances[0] == directData);

    const auto& flushedBytes = instancer.GetLastFlushedBytes();
    CHECK(flushedBytes.size() == sizeof(InstanceData));
    CHECK(flushedBytes.size() == 80);

    // Sprawdzenie zgodnosci binarnej bufora
    InstanceData binaryReconstructed{};
    std::memcpy(&binaryReconstructed, flushedBytes.data(), sizeof(InstanceData));
    CHECK(binaryReconstructed == directData);
}

TEST_CASE("HardwareMeshInstancer - Test 3: Logika dzielnika czestotliwosci SetStreamSourceFreq")
{
    // Weryfikacja stalych D3D9
    CHECK(D3DSTREAMSOURCE_INDEXEDDATA == (1 << 30));
    CHECK(D3DSTREAMSOURCE_INSTANCEDATA == (2 << 30));

    // Weryfikacja funkcji statycznych kalkulacji masek czestotliwosci
    constexpr uint32_t testInstanceCount = 250;
    const uint32_t freq0 = HardwareMeshInstancer::CalculateStream0Frequency(testInstanceCount);
    const uint32_t freq1 = HardwareMeshInstancer::CalculateStream1Frequency();
    const uint32_t freqReset = HardwareMeshInstancer::CalculateStreamResetFrequency();

    CHECK(freq0 == ((1 << 30) | testInstanceCount));
    CHECK(freq1 == ((2 << 30) | 1));
    CHECK(freqReset == 1);

    // Test sekwencji strumieniowania dla dwoch niezaleznych partii
    HardwareMeshInstancer instancer;
    instancer.Initialize(nullptr);

    // Dodanie 200 instancji do bucketa A (mesh 1, mat 0, lod 0)
    InstanceData sampleData{};
    for (int i = 0; i < 200; ++i)
    {
        instancer.AddInstance(1, 0, 0, sampleData);
    }

    // Dodanie 300 instancji do bucketa B (mesh 2, mat 0, lod 0)
    for (int i = 0; i < 300; ++i)
    {
        instancer.AddInstance(2, 0, 0, sampleData);
    }

    CHECK(instancer.GetBucketCount() == 2);
    CHECK(instancer.GetPendingInstanceCount() == 500);

    const uint32_t drawCalls = instancer.Flush(nullptr);
    CHECK(drawCalls == 2);

    const auto& history = instancer.GetStreamFreqHistory();
    // Kazda partia generuje 4 wpisy w historii:
    // 1. Stream 0: INDEXEDDATA | count
    // 2. Stream 1: INSTANCEDATA | 1
    // 3. Stream 0: reset (1)
    // 4. Stream 1: reset (1)
    // Lacznie dla 2 partii: 8 wpisow
    REQUIRE(history.size() == 8);

    // Partia 1 (Bucket A - 200 instancji)
    CHECK(history[0].streamIndex == 0);
    CHECK(history[0].frequencySetting == ((1 << 30) | 200));

    CHECK(history[1].streamIndex == 1);
    CHECK(history[1].frequencySetting == ((2 << 30) | 1));

    CHECK(history[2].streamIndex == 0);
    CHECK(history[2].frequencySetting == 1);

    CHECK(history[3].streamIndex == 1);
    CHECK(history[3].frequencySetting == 1);

    // Partia 2 (Bucket B - 300 instancji)
    CHECK(history[4].streamIndex == 0);
    CHECK(history[4].frequencySetting == ((1 << 30) | 300));

    CHECK(history[5].streamIndex == 1);
    CHECK(history[5].frequencySetting == ((2 << 30) | 1));

    CHECK(history[6].streamIndex == 0);
    CHECK(history[6].frequencySetting == 1);

    CHECK(history[7].streamIndex == 1);
    CHECK(history[7].frequencySetting == 1);
}

TEST_CASE("HardwareMeshInstancer - Test 4: Stress test na 10 000 instancji z redukcja DrawCalli > 98%")
{
    HardwareMeshInstancer instancer;
    const bool initOk = instancer.Initialize(nullptr);
    CHECK(initOk == true);

    SpeedTreeInstancedBridge bridge;

    // 8 typow drzew, kazdy z 2 poziomami LOD -> maksymalnie 16 unikalnych bucketow
    constexpr uint32_t kTreeTypeCount = 8;
    for (uint32_t t = 0; t < kTreeTypeCount; ++t)
    {
        bridge.SetTreeTypeMapping(t, 100 + t, t % 3);
    }

    constexpr uint32_t kStressTotal = 10000;
    for (uint32_t i = 0; i < kStressTotal; ++i)
    {
        const uint32_t treeTypeId = i % kTreeTypeCount;
        const uint32_t lodIndex = (i / 1000) % 2; // LOD 0 lub LOD 1
        const float posX = static_cast<float>((i % 100) * 50);
        const float posY = static_cast<float>((i / 100) * 50);
        const float posZ = static_cast<float>(i % 30);
        const float scale = 0.8f + (static_cast<float>(i % 10) * 0.05f);
        const float yaw = static_cast<float>(i % 628) * 0.01f;
        const uint32_t tint = 0xFF000000 | (i & 0x00FFFFFF);
        const float wind = static_cast<float>(i % 314) * 0.02f;

        bridge.RegisterTree(treeTypeId, posX, posY, posZ, scale, yaw, tint, wind, lodIndex);
    }

    CHECK(bridge.GetRegisteredTreeCount() == kStressTotal);

    // Przeslanie do instancera
    const size_t flushedCount = bridge.FlushTrees(instancer);
    CHECK(flushedCount == kStressTotal);
    CHECK(bridge.GetRegisteredTreeCount() == 0);

    const size_t bucketCount = instancer.GetBucketCount();
    CHECK(bucketCount <= 16);
    CHECK(instancer.GetPendingInstanceCount() == kStressTotal);

    // Wykonanie partii instancji
    const uint32_t drawCalls = instancer.Flush(nullptr);
    CHECK(drawCalls == bucketCount);
    CHECK(drawCalls <= 16);

    CHECK(instancer.GetTotalInstances() == kStressTotal);
    CHECK(instancer.GetTotalDrawCalls() == drawCalls);

    // Weryfikacja redukcji DrawCalli > 98%
    // Dla 16 draw calli na 10 000 instancji redukcja to 1 - 16/10000 = 99.84%
    const double reductionRatio = instancer.GetDrawCallReductionRatio();
    CHECK(reductionRatio > 0.98);
    CHECK(reductionRatio >= 0.9984);

    // Weryfikacja telemetrii bufora pierscieniowego (DynamicGeometryRingBuffer)
    const auto& ring = instancer.GetRingBuffer();
    CHECK(ring.GetTotalBytesStreamed() == static_cast<uint64_t>(kStressTotal) * sizeof(InstanceData));
    CHECK(ring.GetNoOverwriteHits() == drawCalls);
    CHECK(ring.GetDiscardWraps() == 0); // 800 KB miesci sie bezposrednio w buforze 8 MB
    CHECK(ring.GetStallFreeRatio() == 100.0);
}

TEST_CASE("HardwareMeshInstancer - Test 5: Poprawnosc wywolywania MeshDrawCallback przy Flush")
{
    HardwareMeshInstancer instancer;
    REQUIRE(instancer.Initialize(nullptr) == true);

    InstanceData data{};
    instancer.AddInstance(10, 0, 0, data);
    instancer.AddInstance(10, 0, 0, data);
    instancer.AddInstance(20, 1, 0, data);

    uint32_t callbackInvocations = 0;
    uint32_t totalInstancesInCallback = 0;

    const uint32_t dispatched = instancer.Flush(nullptr, [&](const BucketKey& key, uint32_t count) {
        ++callbackInvocations;
        totalInstancesInCallback += count;
        if (key.meshId == 10)
        {
            CHECK(count == 2);
        }
        else if (key.meshId == 20)
        {
            CHECK(count == 1);
        }
    });

    CHECK(dispatched == 2);
    CHECK(callbackInvocations == 2);
    CHECK(totalInstancesInCallback == 3);
}

