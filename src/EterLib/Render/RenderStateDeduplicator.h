#pragma once

#include <cstdint>
#include <span>
#include <functional>

namespace EterLib::Render
{
    struct SortKey
    {
        uint32_t Shader = 0;
        uint32_t Texture = 0;
        uint32_t Material = 0;

        bool operator==(const SortKey& other) const = default;
    };

    struct RenderItem
    {
        SortKey sortKey;
        void* commandData = nullptr;
    };

    struct RenderStats
    {
        uint32_t totalItems = 0;
        
        uint32_t expectedShaderSwitches = 0;
        uint32_t expectedTextureSwitches = 0;
        uint32_t expectedMaterialSwitches = 0;
        
        uint32_t actualShaderSwitches = 0;
        uint32_t actualTextureSwitches = 0;
        uint32_t actualMaterialSwitches = 0;

        uint32_t savedShaderSwitches() const { return expectedShaderSwitches - actualShaderSwitches; }
        uint32_t savedTextureSwitches() const { return expectedTextureSwitches - actualTextureSwitches; }
        uint32_t savedMaterialSwitches() const { return expectedMaterialSwitches - actualMaterialSwitches; }
        
        uint32_t totalSavedSwitches() const { return savedShaderSwitches() + savedTextureSwitches() + savedMaterialSwitches(); }
        uint32_t totalExpectedSwitches() const { return expectedShaderSwitches + expectedTextureSwitches + expectedMaterialSwitches; }

        double getReductionPercentage() const 
        {
            if (totalExpectedSwitches() == 0) return 0.0;
            return (static_cast<double>(totalSavedSwitches()) / totalExpectedSwitches()) * 100.0;
        }
    };

    class RenderStateDeduplicator
    {
    public:
        RenderStateDeduplicator() = default;
        ~RenderStateDeduplicator() = default;

        void ProcessQueue(
            std::span<const RenderItem> sortedQueue,
            const std::function<void(uint32_t)>& bindShader,
            const std::function<void(uint32_t)>& bindTexture,
            const std::function<void(uint32_t)>& bindMaterial,
            const std::function<void(const RenderItem&)>& drawCallback
        );

        void ResetStats();
        const RenderStats& GetStats() const;

    private:
        RenderStats m_stats;
    };
}

