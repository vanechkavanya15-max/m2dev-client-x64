#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <vector>
#include <cmath>
#include <array>

#include "../src/GameLib/Terrain/TerrainCoordinates.h"
#include "../src/GameLib/Terrain/LinearSectorQuadtree.h"
#include "../src/GameLib/Terrain/TerrainHeightStorage.h"
#include "../src/GameLib/Terrain/ChunkStreamingCoordinator.h"
#include "../src/GameLib/Terrain/TerrainSubsystemFacade.h"

using namespace GameLib::Terrain;

TEST_CASE("TerrainCoordinates - Metin2 Domain Validation") {
    SUBCASE("WorldToSector & SectorToWorld") {
        WorldPosition pos1{0.0f, 0.0f, 0.0f};
        SectorCoord sec1 = TerrainMetrics::WorldToSector(pos1);
        CHECK(sec1.x == 0);
        CHECK(sec1.y == 0);

        WorldPosition pos2{25600.0f, 51200.0f, 0.0f};
        SectorCoord sec2 = TerrainMetrics::WorldToSector(pos2);
        CHECK(sec2.x == 1);
        CHECK(sec2.y == 2);

        WorldPosition origin = TerrainMetrics::SectorToWorld(sec2);
        CHECK(origin.x == 25600.0f);
        CHECK(origin.y == 51200.0f);

        WorldPosition center = TerrainMetrics::SectorToWorldCenter(sec2);
        CHECK(center.x == 25600.0f + 12800.0f);
        CHECK(center.y == 51200.0f + 12800.0f);
    }

    SUBCASE("PatchCoord mapping 8x8") {
        SectorCoord sec{1, 1}; // [25600, 25600]
        
        // Lewy dolny patch w sektorze
        WorldPosition p0{25650.0f, 25650.0f, 0.0f};
        PatchCoord patch0 = TerrainMetrics::WorldToLocalPatch(p0, sec);
        CHECK(patch0.x == 0);
        CHECK(patch0.y == 0);

        // Prawy gorny patch w sektorze (3200 * 7 = 22400)
        WorldPosition p7{25600.0f + 25000.0f, 25600.0f + 25000.0f, 0.0f};
        PatchCoord patch7 = TerrainMetrics::WorldToLocalPatch(p7, sec);
        CHECK(patch7.x == 7);
        CHECK(patch7.y == 7);
    }

    SUBCASE("DistanceSqPointToSectorAABB - Player in Sector Never Evicted") {
        SectorCoord sec{1, 1}; // Zakres [25600..51200, 25600..51200]

        // Gracz w srodku sektora
        WorldPosition center{38400.0f, 38400.0f, 0.0f};
        CHECK(TerrainMetrics::DistanceSqPointToSectorAABB(center, sec) == 0.0f);

        // Gracz w prawym gornym rogu sektora (blisko krawedzi 51200)
        WorldPosition farCorner{51199.0f, 51199.0f, 0.0f};
        CHECK(TerrainMetrics::DistanceSqPointToSectorAABB(farCorner, sec) == 0.0f);

        // Punkt 100 jednostek na zewnatrz w prawo
        WorldPosition outsideRight{51300.0f, 38400.0f, 0.0f};
        CHECK(TerrainMetrics::DistanceSqPointToSectorAABB(outsideRight, sec) == 100.0f * 100.0f);
    }
}

TEST_CASE("LinearSectorQuadtree - DOD 85 Nodes & 64 Leaves") {
    LinearSectorQuadtree tree;
    CHECK(LinearSectorQuadtree::TotalNodes == 85);
    CHECK(LinearSectorQuadtree::TotalLeaves == 64);
    CHECK(LinearSectorQuadtree::Depth == 3);

    // Utworz syntetyczne dane wysokosci 131x131 w formacie Metin2
    std::vector<uint16_t> heights(131 * 131, 1000);
    // Ustaw szczyt w patchu (2, 2)
    // Patch 2 to komorki cx in [32..47], cy in [32..47] -> w buforze RAW: +1 -> [33..48]
    heights[35 * 131 + 35] = 4500;

    tree.BuildWithHeights(SectorCoord{0, 0}, heights, 1.0f, 0.0f);
    CHECK(tree.IsBuilt());

    // Korzen (wezel 0) powinien miec minZ ok. 1000 i maxZ co najmniej 4500
    const auto& root = tree.GetNode(0);
    CHECK(root.isLeaf == 0);
    CHECK(root.minZ <= 1000.0f);
    CHECK(root.maxZ >= 4500.0f);

    // Test Frustum Culling - prostopadloscian obejmujacy caly sektor
    // 6 plaszczyzn wokol swiata (Ax + By + Cz + D >= 0 dla wnetrza)
    float frustumInside[6][4] = {
        {  1.0f,  0.0f,  0.0f,   1000.0f },  // x >= -1000
        { -1.0f,  0.0f,  0.0f,  30000.0f },  // x <= 30000
        {  0.0f,  1.0f,  0.0f,   1000.0f },  // y >= -1000
        {  0.0f, -1.0f,  0.0f,  30000.0f },  // y <= 30000
        {  0.0f,  0.0f,  1.0f,  10000.0f },  // z >= -10000
        {  0.0f,  0.0f, -1.0f,  10000.0f }   // z <= 10000
    };

    std::array<uint32_t, 64> visiblePatches{};
    size_t visibleCount = tree.CullTerrainPatches(frustumInside, visiblePatches.data());
    CHECK(visibleCount == 64); // Wszystkie 64 patche powinny byc widoczne!

    // Frustum za plecami (x w zakresie -50000..-40000)
    float frustumOutside[6][4] = {
        {  1.0f,  0.0f,  0.0f,  50000.0f },  // x >= -50000
        { -1.0f,  0.0f,  0.0f, -40000.0f },  // x <= -40000
        {  0.0f,  1.0f,  0.0f,   1000.0f },
        {  0.0f, -1.0f,  0.0f,  30000.0f },
        {  0.0f,  0.0f,  1.0f,  10000.0f },
        {  0.0f,  0.0f, -1.0f,  10000.0f }
    };

    visibleCount = tree.CullTerrainPatches(frustumOutside, visiblePatches.data());
    CHECK(visibleCount == 0); // Zaden patch nie powinien byc widoczny!
}

TEST_CASE("TerrainHeightStorage - Metin2 131x131 Raw Offset & 2-Triangle Math") {
    TerrainHeightStorage storage;

    // Przygotuj sektor [0, 0]
    ChunkHeightData data;
    data.heights.assign(131 * 131, 0);
    data.heightScale = 1.0f;
    data.baseHeight = 0.0f;

    // Ustaw wierzcholki komorki (0, 0)
    // Zgodnie z formatem Metin2, komorka (0, 0) ma wierzcholki w buforze RAW z offsetem +1:
    // Top-Left: (1, 1)
    // Top-Right: (2, 1)
    // Bottom-Left: (1, 2)
    // Bottom-Right: (2, 2)
    // CellSize = 200.0f
    data.heights[1 * 131 + 1] = 100; // Top-Left: h1 = 100
    data.heights[1 * 131 + 2] = 200; // Top-Right: h3 (dla Right triangle) = 200
    data.heights[2 * 131 + 1] = 300; // Bottom-Left: h3 (dla Left triangle) = 300
    data.heights[2 * 131 + 2] = 400; // Bottom-Right: h2 = 400

    storage.InsertChunk(SectorCoord{0, 0}, std::move(data));

    SUBCASE("Top-Left Corner (local 0, 0)") {
        float h = storage.SampleHeight(0.0f, 0.0f);
        CHECK(std::abs(h - 100.0f) < 0.001f);
    }

    SUBCASE("Bottom-Right Corner (local 200, 200)") {
        float h = storage.SampleHeight(200.0f, 200.0f);
        CHECK(std::abs(h - 400.0f) < 0.001f);
    }

    SUBCASE("Left Triangle Sample (xdist <= ydist)") {
        // Punkt (50, 150): xdist = 50, ydist = 150 <= 200 -> Lewy trojkat!
        // xslope = (400 - 300) / 200 = 0.5
        // yslope = (300 - 100) / 200 = 1.0
        // h = 100 + (50 * 0.5 + 150 * 1.0) = 100 + 25 + 150 = 275.0
        float h = storage.SampleHeight(50.0f, 150.0f);
        CHECK(std::abs(h - 275.0f) < 0.001f);
    }

    SUBCASE("Right Triangle Sample (xdist > ydist)") {
        // Punkt (150, 50): xdist = 150, ydist = 50 -> Prawy trojkat!
        // xslope = (200 - 100) / 200 = 0.5
        // yslope = (400 - 200) / 200 = 1.0
        // h = 100 + (150 * 0.5 + 50 * 1.0) = 100 + 75 + 50 = 225.0
        float h = storage.SampleHeight(150.0f, 50.0f);
        CHECK(std::abs(h - 225.0f) < 0.001f);
    }

    SUBCASE("Diagonal Continuity (xdist == ydist)") {
        // Na przekatnej (100, 100) oba trojkaty musza dac dokladnie taki sam wynik!
        // Lewy: 100 + 100*0.5 + 100*1.0 = 250.0
        // Prawy: 100 + 100*0.5 + 100*1.0 = 250.0
        float h = storage.SampleHeight(100.0f, 100.0f);
        CHECK(std::abs(h - 250.0f) < 0.001f);
    }

    SUBCASE("Normal Vector Calculation") {
        auto normalOpt = storage.CalculateNormal(WorldPosition{50.0f, 150.0f, 0.0f});
        CHECK(normalOpt.has_value());
        auto [nx, ny, nz] = normalOpt.value();
        // xslope = 0.5, yslope = 1.0
        // Vector (-0.5, -1.0, 1.0), len = sqrt(0.25 + 1.0 + 1.0) = sqrt(2.25) = 1.5
        // nx = -0.5 / 1.5 = -0.33333, ny = -1.0 / 1.5 = -0.66667, nz = 1.0 / 1.5 = 0.66667
        CHECK(std::abs(nx - (-0.33333f)) < 0.005f);
        CHECK(std::abs(ny - (-0.66667f)) < 0.005f);
        CHECK(std::abs(nz - 0.66667f) < 0.005f);
    }
}

TEST_CASE("ChunkStreamingCoordinator - No Eviction When in Sector") {
    TerrainHeightStorage storage;
    ChunkStreamingCoordinator coordinator(&storage);

    // Gracz stoi w sektorze [1, 1], dokladnie w prawym gornym rogu (bardzo daleko od naroznika 0,0)
    float playerX = 25600.0f + 25000.0f; // 50600
    float playerY = 25600.0f + 25000.0f; // 50600

    // Aktualizacja strumieniowania
    coordinator.UpdateStreaming(playerX, playerY);

    // Sprawdz czy sektor [1, 1] ma stan Queued lub Active (NIE jest Unloaded ani wyewikowany!)
    auto state = coordinator.GetChunkState(SectorCoord{1, 1});
    CHECK(state != ChunkState::Unloaded);

    // Oznacz jako Active
    coordinator.MarkChunkActive(SectorCoord{1, 1});
    CHECK(coordinator.IsChunkLoaded(SectorCoord{1, 1}));

    // Ponowna aktualizacja (nie powinno go wyewikowac!)
    coordinator.UpdateStreaming(playerX, playerY);
    CHECK(coordinator.IsChunkLoaded(SectorCoord{1, 1}));
}

#include <thread>
#include <atomic>

TEST_CASE("Stress Simulation - Multi-Threaded Concurrency") {
    TerrainHeightStorage storage;

    // Utworz dane testowe
    for (int sx = 0; sx < 3; ++sx) {
        for (int sy = 0; sy < 3; ++sy) {
            ChunkHeightData data;
            data.heights.assign(131 * 131, static_cast<uint16_t>((sx + sy) * 100));
            data.heightScale = 1.0f;
            data.baseHeight = 0.0f;
            storage.InsertChunk(SectorCoord{sx, sy}, std::move(data));
        }
    }

    std::atomic<bool> running{true};
    std::atomic<uint64_t> readOperations{0};

    // 8 watkow rownoleglych czytajacych wysokosc, normalne i sprawdzajacych nachylenie
    std::vector<std::thread> readers;
    for (int i = 0; i < 8; ++i) {
        readers.emplace_back([&storage, &running, &readOperations, i]() {
            float x = static_cast<float>(i * 3000);
            float y = static_cast<float>(i * 2000);
            while (running.load(std::memory_order_relaxed)) {
                float h = storage.SampleHeight(x, y);
                (void)h;
                auto norm = storage.CalculateNormal(WorldPosition{x, y, 0.0f});
                (void)norm;
                bool walkable = storage.IsWalkableSlope(x, y, 45.0f);
                (void)walkable;

                std::array<float, 4> xs = {x, x + 10.0f, x + 20.0f, x + 30.0f};
                std::array<float, 4> ys = {y, y + 10.0f, y + 20.0f, y + 30.0f};
                std::array<float, 4> outZ{};
                storage.BatchSampleHeight(xs.data(), ys.data(), outZ.data(), 4);

                readOperations.fetch_add(4, std::memory_order_relaxed);
                x = std::fmod(x + 137.0f, 75000.0f);
                y = std::fmod(y + 193.0f, 75000.0f);
            }
        });
    }

    // 1 watek modyfikujacy kafelki w locie (insert/remove)
    std::thread writer([&storage, &running]() {
        int iter = 0;
        while (running.load(std::memory_order_relaxed)) {
            SectorCoord target{iter % 3, (iter / 3) % 3};
            ChunkHeightData data;
            data.heights.assign(131 * 131, static_cast<uint16_t>(iter * 10));
            data.heightScale = 1.0f;
            data.baseHeight = 0.0f;

            storage.InsertChunk(target, std::move(data));
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            iter++;
        }
    });

    // Pusc symulacje na 200 milisekund
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running.store(false, std::memory_order_relaxed);

    writer.join();
    for (auto& r : readers) {
        r.join();
    }

    CHECK(readOperations.load() > 1000); // Ponad 1000 operacji bez zadnego wyscigu danych ani deadlocka
}

TEST_CASE("Precision Simulation - Extreme Boundary & Multi-Sector Traversal") {
    TerrainHeightStorage storage;

    // Wypelnij 4 sasiednie sektory [0,0], [1,0], [0,1], [1,1]
    // ze spojnymi wierzcholkami na krawedziach
    for (int sy = 0; sy <= 1; ++sy) {
        for (int sx = 0; sx <= 1; ++sx) {
            ChunkHeightData data;
            data.heights.assign(131 * 131, 0);
            data.heightScale = 1.0f;
            data.baseHeight = 0.0f;

            for (uint32_t cy = 0; cy <= 128; ++cy) {
                for (uint32_t cx = 0; cx <= 128; ++cx) {
                    uint32_t rawX = cx + TerrainMetrics::RawBorderOffset;
                    uint32_t rawY = cy + TerrainMetrics::RawBorderOffset;
                    // Globalna wspolrzedna w komorkach
                    uint32_t gX = sx * 128 + cx;
                    uint32_t gY = sy * 128 + cy;
                    // Liniowa funkcja wysokosci swiata: H = gX * 10 + gY * 5
                    data.heights[rawY * 131 + rawX] = static_cast<uint16_t>(gX * 10 + gY * 5);
                }
            }
            storage.InsertChunk(SectorCoord{sx, sy}, std::move(data));
        }
    }

    SUBCASE("Continuous Sampling Across Sector Boundary X=25600") {
        // Przejdz ciagle przez granice od 25500 do 25700 z krokiem 10 jednostek
        for (float x = 25500.0f; x <= 25700.0f; x += 10.0f) {
            float y = 5000.0f;
            float h = storage.SampleHeight(x, y);

            // Oczekiwana wysokosc analityczna:
            // gX = x / 200.0f, gY = y / 200.0f -> H = gX * 10 + gY * 5
            float expectedH = (x / 200.0f) * 10.0f + (y / 200.0f) * 5.0f;
            CHECK(std::abs(h - expectedH) < 0.05f);
        }
    }

    SUBCASE("Continuous Sampling Across Sector Boundary Y=25600") {
        for (float y = 25500.0f; y <= 25700.0f; y += 10.0f) {
            float x = 12000.0f;
            float h = storage.SampleHeight(x, y);

            float expectedH = (x / 200.0f) * 10.0f + (y / 200.0f) * 5.0f;
            CHECK(std::abs(h - expectedH) < 0.05f);
        }
    }

    SUBCASE("Corner Junction (25600, 25600)") {
        float h = storage.SampleHeight(25600.0f, 25600.0f);
        float expectedH = 128.0f * 10.0f + 128.0f * 5.0f; // 1280 + 640 = 1920
        CHECK(std::abs(h - expectedH) < 0.05f);
    }
}

TEST_CASE("Full Gameplay Simulation - Facade Streaming & Dynamic Camera Culling") {
    TerrainSubsystemFacade facade;

    // Zaladuj sektor [0, 0] z terenem
    std::vector<uint16_t> heights(131 * 131, 500);
    facade.LoadSectorData(SectorCoord{0, 0}, std::move(heights), {}, 1.0f, 0.0f);

    // Symulacja ruchu gracza po mapie: od (1000, 1000) do (60000, 60000)
    for (float t = 0.0f; t <= 60000.0f; t += 2000.0f) {
        facade.Update(t, t);
        float h = facade.SampleHeight(t, t);
        (void)h;
    }

    // Test rotacji kamery (Frustum Culling) 360 stopni wokol srodka sektora
    for (int angle = 0; angle < 360; angle += 45) {
        float rad = static_cast<float>(angle) * (3.14159265f / 180.0f);
        float camDirX = std::cos(rad);
        float camDirY = std::sin(rad);

        // Ustaw proste plaszczyzny frustum skierowane w strone camDir
        float frustum[6][4] = {
            { camDirX,  camDirY,  0.0f,  10000.0f },
            {-camDirX, -camDirY,  0.0f,  30000.0f },
            { 0.0f,     0.0f,     1.0f,  5000.0f },
            { 0.0f,     0.0f,    -1.0f,  5000.0f },
            { 1.0f,     0.0f,     0.0f,  50000.0f },
            { 0.0f,     1.0f,     0.0f,  50000.0f }
        };

        std::array<uint32_t, 64> visible{};
        size_t visibleCount = facade.CullSectorPatches(SectorCoord{0, 0}, frustum, visible.data());
        CHECK(visibleCount <= 64);
    }

    facade.Clear();
}
