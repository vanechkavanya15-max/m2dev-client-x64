#pragma once

#include <cstdint>
#include <cmath>
#include <compare>
#include <algorithm>

namespace GameLib::Terrain {

    struct WorldPosition {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};

        [[nodiscard]] bool IsFinite() const noexcept {
            return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
        }
    };

    struct SectorCoord {
        int32_t x{0};
        int32_t y{0};

        constexpr SectorCoord() = default;
        constexpr SectorCoord(int32_t sx, int32_t sy) noexcept : x(sx), y(sy) {}

        auto operator<=>(const SectorCoord&) const = default;
    };

    struct PatchCoord {
        uint8_t x{0}; // Zakres 0..7
        uint8_t y{0}; // Zakres 0..7

        constexpr PatchCoord() = default;
        constexpr PatchCoord(uint8_t px, uint8_t py) noexcept : x(px), y(py) {}

        auto operator<=>(const PatchCoord&) const = default;
    };

    struct HeightCellCoord {
        uint8_t x{0}; // Zakres 0..128
        uint8_t y{0}; // Zakres 0..128

        constexpr HeightCellCoord() = default;
        constexpr HeightCellCoord(uint8_t cx, uint8_t cy) noexcept : x(cx), y(cy) {}

        auto operator<=>(const HeightCellCoord&) const = default;
    };

    class TerrainMetrics {
    public:
        // Stale scisle zgodne z oryginalnym silnikiem Metin2 (PRTerrainLib / AreaTerrain)
        static constexpr float SectorSize = 25600.0f;           // TERRAIN_XSIZE = 128 * 200
        static constexpr float CellSize = 200.0f;               // CELLSCALE = 200 jednostek swiata
        static constexpr uint32_t CellsPerSector = 128;         // TERRAIN_SIZE = 128 komorek
        static constexpr uint32_t HeightmapSize = 131;          // HEIGHTMAP_RAW_XSIZE = 128 + 3
        static constexpr uint32_t HeightmapValidSize = 129;     // HEIGHTMAP_XSIZE = 128 + 1 wierzcholkow
        static constexpr uint32_t RawBorderOffset = 1;          // Punkt (0,0) sektora to (1,1) w pliku RAW
        static constexpr float PatchSize = 3200.0f;             // TERRAIN_PATCHSIZE (16 komorek) * 200 = 3200.0f
        static constexpr uint32_t PatchesPerSide = 8;           // TERRAIN_PATCHCOUNT = 128 / 16 = 8
        static constexpr uint32_t TotalPatchesPerSector = 64;   // 8 * 8 = 64 patche na sektor
        static constexpr uint32_t CellsPerPatch = 16;           // 16 x 16 komorek w jednym patchu

        [[nodiscard]] static inline SectorCoord WorldToSector(const WorldPosition& pos) noexcept {
            return {
                static_cast<int32_t>(std::floor(pos.x / SectorSize)),
                static_cast<int32_t>(std::floor(pos.y / SectorSize))
            };
        }

        [[nodiscard]] static constexpr WorldPosition SectorToWorld(const SectorCoord& coord) noexcept {
            return {
                static_cast<float>(coord.x) * SectorSize,
                static_cast<float>(coord.y) * SectorSize,
                0.0f
            };
        }

        [[nodiscard]] static constexpr WorldPosition SectorToWorldCenter(const SectorCoord& coord) noexcept {
            return {
                (static_cast<float>(coord.x) + 0.5f) * SectorSize,
                (static_cast<float>(coord.y) + 0.5f) * SectorSize,
                0.0f
            };
        }

        [[nodiscard]] static inline PatchCoord WorldToLocalPatch(const WorldPosition& pos, const SectorCoord& sector) noexcept {
            float localX = pos.x - (static_cast<float>(sector.x) * SectorSize);
            float localY = pos.y - (static_cast<float>(sector.y) * SectorSize);

            int32_t px = static_cast<int32_t>(std::floor(localX / PatchSize));
            int32_t py = static_cast<int32_t>(std::floor(localY / PatchSize));

            px = std::clamp(px, 0, static_cast<int32_t>(PatchesPerSide - 1));
            py = std::clamp(py, 0, static_cast<int32_t>(PatchesPerSide - 1));

            return { static_cast<uint8_t>(px), static_cast<uint8_t>(py) };
        }

        [[nodiscard]] static constexpr float DistanceSqPointToSectorAABB(const WorldPosition& pos, const SectorCoord& sector) noexcept {
            float minX = static_cast<float>(sector.x) * SectorSize;
            float maxX = minX + SectorSize;
            float minY = static_cast<float>(sector.y) * SectorSize;
            float maxY = minY + SectorSize;

            float closestX = std::clamp(pos.x, minX, maxX);
            float closestY = std::clamp(pos.y, minY, maxY);

            float dx = pos.x - closestX;
            float dy = pos.y - closestY;
            return (dx * dx) + (dy * dy);
        }
    };

} // namespace GameLib::Terrain

namespace std {
    template<>
    struct hash<GameLib::Terrain::SectorCoord> {
        [[nodiscard]] size_t operator()(const GameLib::Terrain::SectorCoord& c) const noexcept {
            uint64_t packed = (static_cast<uint64_t>(static_cast<uint32_t>(c.x)) << 32) |
                               static_cast<uint32_t>(c.y);
            // 64-bitowy SplitMix Hash
            packed ^= packed >> 30;
            packed *= 0xbf58476d1ce4e5b9ULL;
            packed ^= packed >> 27;
            packed *= 0x94d049bb133111ebULL;
            packed ^= packed >> 31;
            return static_cast<size_t>(packed);
        }
    };
}
