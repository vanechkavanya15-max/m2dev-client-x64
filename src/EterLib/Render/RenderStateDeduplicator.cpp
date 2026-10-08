#ifndef TEST_ENV
#include "../StdAfx.h"
#endif

#include "RenderStateDeduplicator.h"

namespace EterLib::Render
{
    void RenderStateDeduplicator::ProcessQueue(
        std::span<const RenderItem> sortedQueue,
        const std::function<void(uint32_t)>& bindShader,
        const std::function<void(uint32_t)>& bindTexture,
        const std::function<void(uint32_t)>& bindMaterial,
        const std::function<void(const RenderItem&)>& drawCallback)
    {
        m_stats.totalItems += static_cast<uint32_t>(sortedQueue.size());
        m_stats.expectedShaderSwitches += static_cast<uint32_t>(sortedQueue.size());
        m_stats.expectedTextureSwitches += static_cast<uint32_t>(sortedQueue.size());
        m_stats.expectedMaterialSwitches += static_cast<uint32_t>(sortedQueue.size());

        if (sortedQueue.empty())
        {
            return;
        }

        // Initialize state with the first item
        const RenderItem* prevItem = &sortedQueue.front();

        // Bind initial state
        bindShader(prevItem->sortKey.Shader);
        m_stats.actualShaderSwitches++;

        bindTexture(prevItem->sortKey.Texture);
        m_stats.actualTextureSwitches++;

        bindMaterial(prevItem->sortKey.Material);
        m_stats.actualMaterialSwitches++;

        // Draw first item
        drawCallback(*prevItem);

        // Process remaining items
        for (size_t i = 1; i < sortedQueue.size(); ++i)
        {
            const RenderItem& currentItem = sortedQueue[i];

            // Shader deduplication
            if (prevItem->sortKey.Shader != currentItem.sortKey.Shader)
            {
                bindShader(currentItem.sortKey.Shader);
                m_stats.actualShaderSwitches++;
            }

            // Texture deduplication
            if (prevItem->sortKey.Texture != currentItem.sortKey.Texture)
            {
                bindTexture(currentItem.sortKey.Texture);
                m_stats.actualTextureSwitches++;
            }

            // Material deduplication
            if (prevItem->sortKey.Material != currentItem.sortKey.Material)
            {
                bindMaterial(currentItem.sortKey.Material);
                m_stats.actualMaterialSwitches++;
            }

            drawCallback(currentItem);
            prevItem = &currentItem;
        }
    }

    void RenderStateDeduplicator::ResetStats()
    {
        m_stats = RenderStats{};
    }

    const RenderStats& RenderStateDeduplicator::GetStats() const
    {
        return m_stats;
    }
}

