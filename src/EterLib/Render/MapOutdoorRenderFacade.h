#pragma once

#include "RenderQueue.h"
#include "LinearFrameAllocator.h"
#include "../StdAfx.h"
#include <span>
#include <vector>

namespace EterLib::Render
{
    // Mock terrain patch descriptor as we do not have direct access to the actual patch data structure
    struct TerrainPatchDescriptor
    {
        D3DVECTOR minBounds;
        D3DVECTOR maxBounds;
        D3DVECTOR centerPos;
    };

    // Generic interface to satisfy compilation and zero-conflict architectural rules.
    class IFrustumCuller
    {
    public:
        virtual ~IFrustumCuller() = default;
        virtual void UpdateFrustum(const D3DMATRIX& viewProj) = 0;
        virtual bool IsVisible(const D3DVECTOR& minBounds, const D3DVECTOR& maxBounds) const = 0;
    };

    // Generic interface to satisfy compilation and zero-conflict architectural rules.
    class ILODManager
    {
    public:
        virtual ~ILODManager() = default;
        virtual void UpdateLOD(const D3DVECTOR& cameraPos) = 0;
        virtual int GetLODLevel(const D3DVECTOR& patchCenter) const = 0;
    };

    // Generic interface to satisfy compilation and zero-conflict architectural rules.
    class ITerrainPatchBatcher
    {
    public:
        virtual ~ITerrainPatchBatcher() = default;
        virtual void BeginBatch(LinearFrameAllocator& allocator) = 0;
        virtual void AddPatch(const TerrainPatchDescriptor& patchDesc, int lodLevel) = 0;
        virtual void EndBatch(RenderQueue& outQueue) = 0;
    };

    class MapOutdoorRenderFacade
    {
    public:
        MapOutdoorRenderFacade(IFrustumCuller* culler, ILODManager* lodManager, ITerrainPatchBatcher* batcher);
        ~MapOutdoorRenderFacade() = default;

        void SetEnabled(bool enable) noexcept;
        
        // Feed the facade with a list of patches to be processed each frame (since MapOutdoor holds the actual terrain patches).
        void SetPatches(std::span<const TerrainPatchDescriptor> patches);

        void CollectAndSubmit(const D3DMATRIX& viewProj, const D3DVECTOR& cameraPos, 
                              RenderQueue& outQueue, LinearFrameAllocator& allocator);

    private:
        bool m_enabled;
        IFrustumCuller* m_culler;
        ILODManager* m_lodManager;
        ITerrainPatchBatcher* m_batcher;
        std::vector<TerrainPatchDescriptor> m_patches;
    };
}

