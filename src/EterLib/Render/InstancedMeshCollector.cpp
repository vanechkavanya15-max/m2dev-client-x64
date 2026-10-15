#include "InstancedMeshCollector.h"
#include <algorithm>

namespace EterLib::Render
{
    InstancedMeshCollector::InstancedMeshCollector()
        : m_dirty(false)
    {
    }

    InstancedMeshCollector::~InstancedMeshCollector() = default;

    void InstancedMeshCollector::AddInstance(uint32_t meshId, const D3DMATRIX& worldMatrix) noexcept
    {
        // Find existing group or create new one
        auto it = std::find_if(m_tempGroups.begin(), m_tempGroups.end(),
            [meshId](const InternalBatch& batch) { return batch.meshId == meshId; });

        if (it != m_tempGroups.end())
        {
            it->matrices.push_back(worldMatrix);
        }
        else
        {
            InternalBatch newBatch;
            newBatch.meshId = meshId;
            newBatch.matrices.push_back(worldMatrix);
            m_tempGroups.push_back(std::move(newBatch));
        }

        m_dirty = true;
    }

    std::span<const InstanceBatch> InstancedMeshCollector::GetBatches() const noexcept
    {
        if (m_dirty)
        {
            m_batches.clear();
            m_linearMatrices.clear();

            // Calculate total matrices for single allocation
            size_t totalMatrices = 0;
            for (const auto& group : m_tempGroups)
            {
                totalMatrices += group.matrices.size();
            }

            m_linearMatrices.reserve(totalMatrices);
            m_batches.reserve(m_tempGroups.size());

            // Flatten data
            for (const auto& group : m_tempGroups)
            {
                const size_t startOffset = m_linearMatrices.size();
                m_linearMatrices.insert(m_linearMatrices.end(), group.matrices.begin(), group.matrices.end());
                
                InstanceBatch batch;
                batch.meshId = group.meshId;
                batch.matrices = std::span<const D3DMATRIX>(m_linearMatrices.data() + startOffset, group.matrices.size());
                m_batches.push_back(batch);
            }

            m_dirty = false;
        }

        return m_batches;
    }

    void InstancedMeshCollector::Reset() noexcept
    {
        m_tempGroups.clear();
        m_batches.clear();
        m_linearMatrices.clear();
        m_dirty = false;
    }
}

