#pragma once

#include <span>
#include <cstdint>

namespace EterLib::Render
{
    class RenderQueue;
    class LinearFrameAllocator;

    /**
     * @brief Batcher for submitting visible terrain patches to the render queue.
     */
    class TerrainPatchBatcher
    {
    public:
        TerrainPatchBatcher() = default;
        ~TerrainPatchBatcher() = default;

        // Generator komend DrawIndexedCommand dla widocznych kafelkow terenu zasilajacy RenderQueue.
        void SubmitPatches(std::span<const uint32_t> visibleNodeIds, RenderQueue& targetQueue, LinearFrameAllocator& allocator);

    private:
        static constexpr uint32_t TERRAIN_SHADER_ID = 42; // Identifier used in the sort key
    };
} // namespace EterLib::Render

