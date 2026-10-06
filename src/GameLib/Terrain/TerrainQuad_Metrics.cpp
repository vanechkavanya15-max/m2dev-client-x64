#include "../StdAfx.h"
#include "ITerrainQuadtreeCuller.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/ModernLogger.h"
#include "../../UserInterface/Core/EventBus.h"
#include <memory>
#include <string_view>

namespace GameLib::Terrain
{
    // Custom event to report telemetry data via the EventBus
    struct TerrainTelemetryEvent : public UserInterface::Core::IEvent
    {
        uint32_t visibleTriangles;
        uint32_t culledPatches;
        uint32_t renderedPatches;
        
        TerrainTelemetryEvent(uint32_t visibleTris, uint32_t culled, uint32_t rendered)
            : visibleTriangles(visibleTris), culledPatches(culled), renderedPatches(rendered) {}
    };

    /**
     * @class TerrainQuad_Metrics
     * @brief Implementation of ITerrainQuadtreeCuller handling telemetry of visible triangles and culled patches.
     * Adheres to the Zero-Conflict and High-Performance architecture rules.
     */
    class TerrainQuad_Metrics final : public ITerrainQuadtreeCuller
    {
    public:
        TerrainQuad_Metrics() = default;
        ~TerrainQuad_Metrics() override = default;

        void BuildQuadtree(int32_t sectorX, int32_t sectorY) override
        {
            EterBase::ModernLogger::Info("Building Quadtree metrics for sector [{}, {}]", sectorX, sectorY);
            // Metric initialization logic here
            totalPatches_ = 256; // Mock total number of patches in a quadtree
        }

        size_t CullTerrainPatches(const float viewFrustum[6][4], uint32_t* outVisiblePatchIds) override
        {
            EterBase::ModernLogger::Debug("Culling terrain patches based on view frustum...");
            
            // Dummy logic for metrics
            uint32_t renderedPatches = 120;
            uint32_t culledPatches = totalPatches_ - renderedPatches;
            uint32_t visibleTriangles = renderedPatches * 512; // Assume 512 triangles per patch
            
            // Collect the visible patch IDs
            for (size_t i = 0; i < renderedPatches; ++i)
            {
                outVisiblePatchIds[i] = static_cast<uint32_t>(i); // Mock ID
            }
            
            // Publish telemetry metrics event to GUI / other subsystems
            UserInterface::Core::EventBus::GetInstance().Publish(
                TerrainTelemetryEvent(visibleTriangles, culledPatches, renderedPatches)
            );
            
            return renderedPatches;
        }

        uint8_t SelectLOD(float distance) const override
        {
            // Simple LOD selection metric
            if (distance < 1000.0f) return 0;
            if (distance < 5000.0f) return 1;
            return 2;
        }

        void Clear() override
        {
            EterBase::ModernLogger::Info("Clearing Terrain Quadtree Metrics state.");
            totalPatches_ = 0;
        }
        
    private:
        uint32_t totalPatches_ = 0;
    };

    /**
     * @brief Factory function to create the metrics quadtree culler instance.
     * Implements mandatory modern C++ error handling using std::expected.
     * @return EterBase::Result containing unique_ptr on success, or an error string on failure.
     */
    EterBase::Result<std::unique_ptr<ITerrainQuadtreeCuller>, std::string_view> CreateTerrainMetricsCuller()
    {
        try {
            auto instance = std::make_unique<TerrainQuad_Metrics>();
            return instance;
        } catch (...) {
            return EterBase::MakeError(std::string_view("Failed to allocate TerrainQuad_Metrics"));
        }
    }
}
