#pragma once

#include <d3d9.h>
#include <vector>
#include <cstdint>

namespace EterLib::Render
{
    class RenderQueue;
    class LinearFrameAllocator;

    /**
     * @brief Batches rendering of shadows under characters into a single instanced call.
     */
    class InstancedShadowBatcher
    {
    public:
        InstancedShadowBatcher() = default;
        ~InstancedShadowBatcher() = default;

        // Non-copyable
        InstancedShadowBatcher(const InstancedShadowBatcher&) = delete;
        InstancedShadowBatcher& operator=(const InstancedShadowBatcher&) = delete;

        /**
         * @brief Adds a shadow to the batch.
         * @param x X coordinate.
         * @param y Y coordinate.
         * @param z Z coordinate.
         * @param radius Radius of the shadow.
         * @param alpha Alpha transparency of the shadow.
         */
        void AddShadow(float x, float y, float z, float radius, float alpha);

        /**
         * @brief Submits the batched shadows to the render queue.
         * @param queue The render queue.
         * @param allocator The frame allocator to allocate command memory.
         */
        void Submit(RenderQueue& queue, LinearFrameAllocator& allocator);

        /**
         * @brief Clears the batch.
         */
        void Clear() noexcept;

    private:
        struct Vertex
        {
            float x, y, z;
            uint32_t color;
            float u, v;
        };

        std::vector<Vertex> m_vertices;
    };
}

