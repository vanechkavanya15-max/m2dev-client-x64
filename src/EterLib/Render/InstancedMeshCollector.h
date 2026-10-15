#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include "../StdAfx.h"

namespace EterLib::Render
{
    struct InstanceBatch
    {
        uint32_t meshId;
        std::span<const D3DMATRIX> matrices;
    };

    class InstancedMeshCollector
    {
    public:
        InstancedMeshCollector();
        ~InstancedMeshCollector();

        void AddInstance(uint32_t meshId, const D3DMATRIX& worldMatrix) noexcept;
        std::span<const InstanceBatch> GetBatches() const noexcept;
        void Reset() noexcept;

    private:
        struct InternalBatch
        {
            uint32_t meshId;
            std::vector<D3DMATRIX> matrices;
        };

        mutable std::vector<InternalBatch> m_tempGroups;
        mutable std::vector<InstanceBatch> m_batches;
        mutable std::vector<D3DMATRIX> m_linearMatrices;
        mutable bool m_dirty;
    };
}

