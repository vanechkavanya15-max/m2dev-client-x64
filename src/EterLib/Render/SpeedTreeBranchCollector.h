#pragma once

#ifndef TEST_MOCK_D3D9
#include <d3d9.h>
#else
#include "d3d9.h"
#endif

#include "RenderQueue.h"
#include "LinearFrameAllocator.h"
#include <cstdint>

namespace EterLib::Render
{
    class SpeedTreeBranchCollector
    {
    public:
        SpeedTreeBranchCollector() = default;
        ~SpeedTreeBranchCollector() = default;

        SpeedTreeBranchCollector(const SpeedTreeBranchCollector&) = delete;
        SpeedTreeBranchCollector& operator=(const SpeedTreeBranchCollector&) = delete;

        void SubmitBranch(LPDIRECT3DVERTEXBUFFER9 vb, 
                          LPDIRECT3DINDEXBUFFER9 ib, 
                          uint32_t numPrims, 
                          const D3DMATRIX& world, 
                          RenderQueue& queue, 
                          LinearFrameAllocator& alloc);
    };
}

