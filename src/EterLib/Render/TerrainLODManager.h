#pragma once

#include <cstdint>
#include <span>
#include <array>

namespace EterLib::Render
{
    class TerrainLODManager
    {
    public:
        TerrainLODManager() = default;
        ~TerrainLODManager() = default;

        TerrainLODManager(const TerrainLODManager&) = delete;
        TerrainLODManager& operator=(const TerrainLODManager&) = delete;

        void SetLODDistances(std::span<const float, 4> distances) noexcept;

        [[nodiscard]] uint32_t CalculateLOD(float distance) const noexcept;

        [[nodiscard]] uint32_t GetStitchMask(
            uint32_t currentLOD,
            uint32_t northLOD,
            uint32_t southLOD,
            uint32_t eastLOD,
            uint32_t westLOD) const noexcept;

    private:
        std::array<float, 4> m_lodDistances{};
    };
}

