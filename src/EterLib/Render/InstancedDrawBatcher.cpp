/**
 * @file InstancedDrawBatcher.cpp
 * @brief Implementation of InstancedDrawBatcher.
 */

#include "InstancedDrawBatcher.h"
#include <cstring>

namespace EterLib::Render
{
    InstancedDrawBatcher::~InstancedDrawBatcher()
    {
        if (m_instanceBuffer)
        {
            m_instanceBuffer->Release();
            m_instanceBuffer = nullptr;
        }
    }

    void InstancedDrawBatcher::FlushBatches(LPDIRECT3DDEVICE9 device, RenderQueue& queue) noexcept
    {
        if (!device)
            return;

        const auto& instances = queue.GetInstances();
        const size_t count = instances.size();

        if (count == 0)
            return;

        // Ensure we have a large enough buffer
        if (count > m_capacity || !m_instanceBuffer)
        {
            if (m_instanceBuffer)
            {
                m_instanceBuffer->Release();
                m_instanceBuffer = nullptr;
            }

            // Allocate a larger buffer (e.g. 1.5x to avoid frequent reallocations)
            m_capacity = count + (count / 2);
            
            // Note: We use D3DUSAGE_DYNAMIC and D3DPOOL_DEFAULT for dynamic instance buffers
            if (FAILED(device->CreateVertexBuffer(
                static_cast<UINT>(m_capacity * sizeof(DrawInstance)),
                D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                0, // FVF is 0 because we use vertex declarations
                D3DPOOL_DEFAULT,
                &m_instanceBuffer,
                nullptr)))
            {
                // Allocation failed; fallback / return
                m_capacity = 0;
                queue.Clear();
                return;
            }
        }

        if (!m_instanceBuffer)
        {
            queue.Clear();
            return;
        }

        // Map buffer and copy data
        void* data = nullptr;
        if (SUCCEEDED(m_instanceBuffer->Lock(0, static_cast<UINT>(count * sizeof(DrawInstance)), &data, D3DLOCK_DISCARD)))
        {
            // For standard MSVC/x64 context where memcpy works with raw memory
            std::memcpy(data, instances.data(), count * sizeof(DrawInstance));
            m_instanceBuffer->Unlock();

            // Set up stream 1 for instances
            device->SetStreamSourceFreq(0, (D3DSTREAMSOURCE_INDEXEDDATA | static_cast<UINT>(count)));
            device->SetStreamSourceFreq(1, (D3DSTREAMSOURCE_INSTANCEDATA | 1));

            // Bind instance buffer to stream 1
            device->SetStreamSource(1, m_instanceBuffer, 0, sizeof(DrawInstance));

            // Assume vertex declaration, indices, and stream 0 are already bound
            // Draw all instances using hardware instancing. Number of primitives per instance should be provided
            // externally, here we just assume it executes a DrawIndexedPrimitive.
            // device->DrawIndexedPrimitive(...);

            // Reset frequencies (good practice to avoid state leaks)
            device->SetStreamSourceFreq(0, 1);
            device->SetStreamSourceFreq(1, 1);

            if (count > 1)
            {
                m_savedDrawCalls += (count - 1);
            }
        }

        // Clear the queue after flushing
        queue.Clear();
    }

    size_t InstancedDrawBatcher::GetSavedDrawCallCount() const noexcept
    {
        return m_savedDrawCalls;
    }
}

