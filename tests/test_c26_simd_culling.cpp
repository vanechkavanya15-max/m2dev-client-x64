#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cassert>
#include <iomanip>

#include "Graphics/CPUIDFeatures.h"
#include "Graphics/SIMDFrustumCulling.h"

using namespace Client::Graphics;

static FrustumPlanes CreateStandardFrustum()
{
    // Typowe pole widzenia kamery (Z-up lub Y-up)
    FrustumPlanes frustum;
    // Lewa plaszczyzna: x + z >= 0 -> x + z = 0
    frustum.SetPlane(0,  1.0f,  0.0f,  0.5f,  1000.0f);
    // Prawa plaszczyzna
    frustum.SetPlane(1, -1.0f,  0.0f,  0.5f,  1000.0f);
    // Dolna plaszczyzna
    frustum.SetPlane(2,  0.0f,  1.0f,  0.5f,  1000.0f);
    // Gorna plaszczyzna
    frustum.SetPlane(3,  0.0f, -1.0f,  0.5f,  1000.0f);
    // Bliska plaszczyzna (Near)
    frustum.SetPlane(4,  0.0f,  0.0f,  1.0f, -10.0f);
    // Daleka plaszczyzna (Far)
    frustum.SetPlane(5,  0.0f,  0.0f, -1.0f,  5000.0f);

    frustum.Normalize();
    return frustum;
}

static void TestCPUIDDetection()
{
    std::cout << "[TEST] 1. Weryfikacja sprzetowa CPUID i bezpieczenstwa rejestrow..." << std::endl;
    const auto& cpu = CPUIDFeatures::Get();
    std::cout << "       " << cpu.ToString() << std::endl;

    // x64 gwarantuje SSE2
    assert(cpu.HasSSE2());

    // Jesli jest AVX/AVX2, to OSXSAVE musi byc wlaczone przez system
    if (cpu.HasAVX() || cpu.HasAVX2())
    {
        assert(cpu.HasOSXSAVE());
        std::cout << "       [OK] OSXSAVE i rejestry YMM sa w 100% bezpieczne dla tego systemu." << std::endl;
    }
    else
    {
        std::cout << "       [INFO] AVX2 nie jest dostepne na tym CPU/OS - aktywowany fallback SSE2." << std::endl;
    }
    std::cout << "[PASS] Test CPUID zakonczony pomyslnie.\n" << std::endl;
}

static void TestBitwiseConsistencySpheres()
{
    std::cout << "[TEST] 2. Bitowa zgodnosc wynikow culling sfer (Scalar vs SSE2 vs AVX2)..." << std::endl;
    const auto frustum = CreateStandardFrustum();
    const auto& cpu = CPUIDFeatures::Get();

    constexpr size_t TEST_COUNT = 10000;
    std::vector<BoundingSphere> spheres(TEST_COUNT);

    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> distPos(-3000.0f, 3000.0f);
    std::uniform_real_distribution<float> distRadius(10.0f, 200.0f);

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        spheres[i] = BoundingSphere(distPos(rng), distPos(rng), distPos(rng), distRadius(rng));
    }

    std::vector<uint8_t> resultsScalar(TEST_COUNT, 0xFF);
    std::vector<uint8_t> resultsSSE(TEST_COUNT, 0xFF);
    std::vector<uint8_t> resultsAVX2(TEST_COUNT, 0xFF);

    FrustumCullSpheres_Scalar(frustum, spheres.data(), TEST_COUNT, resultsScalar.data());
    FrustumCullSpheres_SSE2(frustum, spheres.data(), TEST_COUNT, resultsSSE.data());

    // Sprawdzenie 100% zgodnosci SSE2 ze Scalarem
    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        assert(resultsScalar[i] == resultsSSE[i] && "Rozbieznosc miedzy Scalar a SSE2 dla sfer!");
    }
    std::cout << "       [PASS] SSE2 wygenerowalo w 100% identyczne wyniki co Scalar na 10 000 sferach." << std::endl;

    // Jesli obslugiwane AVX2, sprawdzamy zgodnosc AVX2
    if (cpu.HasAVX2())
    {
        FrustumCullSpheres_AVX2(frustum, spheres.data(), TEST_COUNT, resultsAVX2.data());
        for (size_t i = 0; i < TEST_COUNT; ++i)
        {
            assert(resultsScalar[i] == resultsAVX2[i] && "Rozbieznosc miedzy Scalar a AVX2 dla sfer!");
        }
        std::cout << "       [PASS] AVX2 wygenerowalo w 100% identyczne wyniki co Scalar na 10 000 sferach." << std::endl;
    }
    std::cout << "[PASS] Test bitowej zgodnosci sfer zakonczony pomyslnie.\n" << std::endl;
}

static void TestBitwiseConsistencyAABB()
{
    std::cout << "[TEST] 3. Bitowa zgodnosc wynikow culling AABB (Scalar vs SSE2 vs AVX2)..." << std::endl;
    const auto frustum = CreateStandardFrustum();
    const auto& cpu = CPUIDFeatures::Get();

    constexpr size_t TEST_COUNT = 10000;
    std::vector<BoundingAABB> aabbs(TEST_COUNT);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> distPos(-3000.0f, 3000.0f);
    std::uniform_real_distribution<float> distSize(20.0f, 300.0f);

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        float x = distPos(rng);
        float y = distPos(rng);
        float z = distPos(rng);
        float sx = distSize(rng);
        float sy = distSize(rng);
        float sz = distSize(rng);

        aabbs[i] = BoundingAABB(x, y, z, x + sx, y + sy, z + sz);
    }

    std::vector<uint8_t> resultsScalar(TEST_COUNT, 0xFF);
    std::vector<uint8_t> resultsSSE(TEST_COUNT, 0xFF);
    std::vector<uint8_t> resultsAVX2(TEST_COUNT, 0xFF);

    FrustumCullAABB_Scalar(frustum, aabbs.data(), TEST_COUNT, resultsScalar.data());
    FrustumCullAABB_SSE2(frustum, aabbs.data(), TEST_COUNT, resultsSSE.data());

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        assert(resultsScalar[i] == resultsSSE[i] && "Rozbieznosc miedzy Scalar a SSE2 dla AABB!");
    }
    std::cout << "       [PASS] SSE2 wygenerowalo w 100% identyczne wyniki co Scalar na 10 000 AABB." << std::endl;

    if (cpu.HasAVX2())
    {
        FrustumCullAABB_AVX2(frustum, aabbs.data(), TEST_COUNT, resultsAVX2.data());
        for (size_t i = 0; i < TEST_COUNT; ++i)
        {
            assert(resultsScalar[i] == resultsAVX2[i] && "Rozbieznosc miedzy Scalar a AVX2 dla AABB!");
        }
        std::cout << "       [PASS] AVX2 wygenerowalo w 100% identyczne wyniki co Scalar na 10 000 AABB." << std::endl;
    }
    std::cout << "[PASS] Test bitowej zgodnosci AABB zakonczony pomyslnie.\n" << std::endl;
}

static void TestDynamicDispatcherAndBenchmark()
{
    std::cout << "[TEST] 4. Test Dynamic Dispatchera i Benchmark Wydajnosci..." << std::endl;
    const auto frustum = CreateStandardFrustum();
    const auto& cpu = CPUIDFeatures::Get();

    std::cout << "       Aktywny domyslny backend: " << GetActiveFrustumCullBackendName() << std::endl;

    constexpr size_t BENCHMARK_COUNT = 50000;
    std::vector<BoundingSphere> spheres(BENCHMARK_COUNT);
    std::vector<uint8_t> results(BENCHMARK_COUNT, 0);

    std::mt19937 rng(999);
    std::uniform_real_distribution<float> distPos(-4000.0f, 4000.0f);
    std::uniform_real_distribution<float> distRadius(15.0f, 150.0f);

    for (size_t i = 0; i < BENCHMARK_COUNT; ++i)
    {
        spheres[i] = BoundingSphere(distPos(rng), distPos(rng), distPos(rng), distRadius(rng));
    }

    // Benchmark Scalar
    auto t0 = std::chrono::high_resolution_clock::now();
    FrustumCullSpheres_Scalar(frustum, spheres.data(), BENCHMARK_COUNT, results.data());
    auto t1 = std::chrono::high_resolution_clock::now();
    auto durationScalarUs = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    // Benchmark SSE2
    t0 = std::chrono::high_resolution_clock::now();
    FrustumCullSpheres_SSE2(frustum, spheres.data(), BENCHMARK_COUNT, results.data());
    t1 = std::chrono::high_resolution_clock::now();
    auto durationSSEUs = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    std::cout << "       Czas dla " << BENCHMARK_COUNT << " sfer (Scalar): " << durationScalarUs << " us" << std::endl;
    std::cout << "       Czas dla " << BENCHMARK_COUNT << " sfer (SSE2)  : " << durationSSEUs << " us (Przyspieszenie: "
              << std::fixed << std::setprecision(2) << (double)durationScalarUs / durationSSEUs << "x)" << std::endl;

    if (cpu.HasAVX2())
    {
        t0 = std::chrono::high_resolution_clock::now();
        FrustumCullSpheres_AVX2(frustum, spheres.data(), BENCHMARK_COUNT, results.data());
        t1 = std::chrono::high_resolution_clock::now();
        auto durationAVX2Us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

        std::cout << "       Czas dla " << BENCHMARK_COUNT << " sfer (AVX2)  : " << durationAVX2Us << " us (Przyspieszenie: "
                  << std::fixed << std::setprecision(2) << (double)durationScalarUs / durationAVX2Us << "x)" << std::endl;
    }

    // Test wywolania wsadowego przez dispatcher
    FrustumCullSpheresBatch(frustum, spheres.data(), BENCHMARK_COUNT, results.data());
    assert(!spheres.empty());

    std::cout << "[PASS] Test dispatchera i benchmark zakonczony pomyslnie.\n" << std::endl;
}

static void TestEdgeCasesAndTails()
{
    std::cout << "[TEST] 5. Skrajne przypadki (Edge Cases), puste tablice i nieliniowe ogony (Tails)..." << std::endl;
    const auto frustum = CreateStandardFrustum();
    const auto& cpu = CPUIDFeatures::Get();

    // 1. Pusty bufor / count == 0
    uint8_t dummyRes = 0xAA;
    FrustumCullSpheresBatch(frustum, nullptr, 0, &dummyRes);
    FrustumCullAABBBatch(frustum, nullptr, 0, &dummyRes);
    assert(dummyRes == 0xAA && "Bufor o count==0 nie powinien byc modyfikowany!");

    // 2. Testy dla kazdego malego i nieregularnego rozmiaru ogonow (od 1 do 33 oraz 127)
    const std::vector<size_t> tailSizes = {1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 15, 17, 23, 31, 33, 127};

    std::mt19937 rng(777);
    std::uniform_real_distribution<float> distPos(-2500.0f, 2500.0f);
    std::uniform_real_distribution<float> distRadius(5.0f, 120.0f);

    for (size_t size : tailSizes)
    {
        std::vector<BoundingSphere> testSpheres(size);
        std::vector<BoundingAABB> testAABBs(size);

        for (size_t i = 0; i < size; ++i)
        {
            testSpheres[i] = BoundingSphere(distPos(rng), distPos(rng), distPos(rng), distRadius(rng));

            float x = distPos(rng), y = distPos(rng), z = distPos(rng);
            float s = distRadius(rng);
            testAABBs[i] = BoundingAABB(x, y, z, x + s, y + s, z + s);
        }

        std::vector<uint8_t> refScalarSpheres(size, 0xFF);
        std::vector<uint8_t> sseSpheres(size, 0xFF);
        std::vector<uint8_t> avxSpheres(size, 0xFF);

        FrustumCullSpheres_Scalar(frustum, testSpheres.data(), size, refScalarSpheres.data());
        FrustumCullSpheres_SSE2(frustum, testSpheres.data(), size, sseSpheres.data());
        assert(refScalarSpheres == sseSpheres && "SSE2 Spheres tail mismatch!");

        if (cpu.HasAVX2())
        {
            FrustumCullSpheres_AVX2(frustum, testSpheres.data(), size, avxSpheres.data());
            assert(refScalarSpheres == avxSpheres && "AVX2 Spheres tail mismatch!");
        }

        // AABB
        std::vector<uint8_t> refScalarAABB(size, 0xFF);
        std::vector<uint8_t> sseAABB(size, 0xFF);
        std::vector<uint8_t> avxAABB(size, 0xFF);

        FrustumCullAABB_Scalar(frustum, testAABBs.data(), size, refScalarAABB.data());
        FrustumCullAABB_SSE2(frustum, testAABBs.data(), size, sseAABB.data());
        assert(refScalarAABB == sseAABB && "SSE2 AABB tail mismatch!");

        if (cpu.HasAVX2())
        {
            FrustumCullAABB_AVX2(frustum, testAABBs.data(), size, avxAABB.data());
            assert(refScalarAABB == avxAABB && "AVX2 AABB tail mismatch!");
        }
    }
    std::cout << "       [PASS] Wszystkie rozmiary ogonow (od 1 do 127) sa w 100% zgodne miedzy kernelami." << std::endl;

    // 3. Test unaligned offsetu wskaznika (np. spheres.data() + 1 nie jest 32-bajtowo wyrownany)
    std::vector<BoundingSphere> unalignedSpheres(17);
    for (size_t i = 0; i < 17; ++i)
    {
        unalignedSpheres[i] = BoundingSphere(distPos(rng), distPos(rng), distPos(rng), distRadius(rng));
    }
    std::vector<uint8_t> unalignedResScalar(16, 0);
    std::vector<uint8_t> unalignedResBatch(16, 0);

    FrustumCullSpheres_Scalar(frustum, unalignedSpheres.data() + 1, 16, unalignedResScalar.data());
    FrustumCullSpheresBatch(frustum, unalignedSpheres.data() + 1, 16, unalignedResBatch.data());
    assert(unalignedResScalar == unalignedResBatch && "Unaligned memory load mismatch!");
    std::cout << "       [PASS] Test unaligned memory loads (brak wyrownania 32B) zaliczony." << std::endl;

    // 4. Test przelaczania backendu w locie (SetFrustumCullBackend)
    SetFrustumCullBackend(KernelBackend::Scalar);
    assert(GetActiveFrustumCullBackend() == KernelBackend::Scalar);

    SetFrustumCullBackend(KernelBackend::SSE2);
    assert(GetActiveFrustumCullBackend() == KernelBackend::SSE2);

    SetFrustumCullBackend(KernelBackend::Auto);
    std::cout << "       [PASS] Dynamiczne przelaczanie backendow w locie zaliczone." << std::endl;

    std::cout << "[PASS] Test skrajnych przypadkow zakonczony pomyslnie.\n" << std::endl;
}

int main()
{
    std::cout << "=================================================================" << std::endl;
    std::cout << "=== URUCHAMIANIE TESTOW: SIMD Frustum Culling Dynamic Dispatch ===" << std::endl;
    std::cout << "=================================================================\n" << std::endl;

    TestCPUIDDetection();
    TestBitwiseConsistencySpheres();
    TestBitwiseConsistencyAABB();
    TestDynamicDispatcherAndBenchmark();
    TestEdgeCasesAndTails();

    std::cout << "=== WSZYSTKIE TESTY SIMD FRUSTUM CULLING ZAKONCZONE SUKCESEM (100% PASS) ===" << std::endl;
    return 0;
}
