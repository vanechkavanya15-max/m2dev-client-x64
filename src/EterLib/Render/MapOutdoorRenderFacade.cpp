#include "MapOutdoorRenderFacade.h"

namespace EterLib::Render
{
    MapOutdoorRenderFacade::MapOutdoorRenderFacade(IFrustumCuller* culler, ILODManager* lodManager, ITerrainPatchBatcher* batcher)
        : m_enabled(true),
          m_culler(culler),
          m_lodManager(lodManager),
          m_batcher(batcher)
    {
    }

    void MapOutdoorRenderFacade::SetEnabled(bool enable) noexcept
    {
        m_enabled = enable;
    }

    void MapOutdoorRenderFacade::SetPatches(std::span<const TerrainPatchDescriptor> patches)
    {
        m_patches.assign(patches.begin(), patches.end());
    }

    void MapOutdoorRenderFacade::CollectAndSubmit(const D3DMATRIX& viewProj, const D3DVECTOR& cameraPos, 
                                                  RenderQueue& outQueue, LinearFrameAllocator& allocator)
    {
        if (!m_enabled)
        {
            return;
        }

        if (m_culler)
        {
            m_culler->UpdateFrustum(viewProj);
        }

        if (m_lodManager)
        {
            m_lodManager->UpdateLOD(cameraPos);
        }

        if (m_batcher)
        {
            m_batcher->BeginBatch(allocator);

            for (const auto& patch : m_patches)
            {
                bool isVisible = true;
                if (m_culler)
                {
                    isVisible = m_culler->IsVisible(patch.minBounds, patch.maxBounds);
                }

                if (isVisible)
                {
                    int lodLevel = 0;
                    if (m_lodManager)
                    {
                        lodLevel = m_lodManager->GetLODLevel(patch.centerPos);
                    }

                    m_batcher->AddPatch(patch, lodLevel);
                }
            }

            m_batcher->EndBatch(outQueue);
        }
    }
}

