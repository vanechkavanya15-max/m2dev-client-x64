/**
 * @file InstancedDrawBatcher.h
 * @brief Batcher for instanced rendering in Direct3D 9.
 */
#pragma once

#include <d3d9.h>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace EterLib::Render
{
    /**
     * @brief Data for a single instance.
     */
    struct DrawInstance
    {
        float transform[16];
        uint32_t color;
    };

    /**
     * @brief A simple queue to collect draw instances before batching.
     */
    class RenderQueue
    {
    public:
        /**
         * @brief Adds a new instance to the queue.
         * @param instance The instance data.
         */
        void AddInstance(const DrawInstance& instance)
        {
            m_instances.push_back(instance);
        }

        /**
         * @brief Clears the queue.
         */
        void Clear() noexcept
        {
            m_instances.clear();
        }

        /**
         * @brief Returns all queued instances.
         * @return Const reference to the underlying vector.
         */
        [[nodiscard]] const std::vector<DrawInstance>& GetInstances() const noexcept
        {
            return m_instances;
        }

    private:
        std::vector<DrawInstance> m_instances;
    };

    /**
     * @brief Batches multiple draw instances into a single draw call.
     * 
     * Uses hardware instancing (second vertex stream or VS constants) to draw
     * multiple instances in a single DrawPrimitive call, reducing CPU overhead.
     */
    class InstancedDrawBatcher
    {
    public:
        InstancedDrawBatcher() noexcept = default;
        ~InstancedDrawBatcher();

        // Non-copyable
        InstancedDrawBatcher(const InstancedDrawBatcher&) = delete;
        InstancedDrawBatcher& operator=(const InstancedDrawBatcher&) = delete;

        /**
         * @brief Submits all instances in the queue as a single batched draw call.
         * @param device Direct3D 9 device.
         * @param queue The queue containing instances to flush.
         */
        void FlushBatches(LPDIRECT3DDEVICE9 device, RenderQueue& queue) noexcept;

        /**
         * @brief Gets the number of draw calls saved by batching.
         * @return Number of saved draw calls.
         */
        [[nodiscard]] size_t GetSavedDrawCallCount() const noexcept;

    private:
        LPDIRECT3DVERTEXBUFFER9 m_instanceBuffer = nullptr;
        size_t m_capacity = 0;
        size_t m_savedDrawCalls = 0;
    };
}

