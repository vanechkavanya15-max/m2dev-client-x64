#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#endif

#include "LinearFrameAllocator.h"
#include "RenderQueue.h"
#include "FrameStatisticsTracker.h"
#include "DynamicRingVertexBuffer.h"

#include "OpaquePassDispatcher.h"
#include "AlphaTestPassDispatcher.h"
#include "AlphaBlendPassDispatcher.h"
#include "AdditivePassDispatcher.h"
#include "DepthPrepassDispatcher.h"
#include "UIPassDispatcher.h"

namespace EterLib::Render
{
    /**
     * @brief Orchestrator for the rendering frame pipeline.
     */
    class FramePipelineCoordinator
    {
    public:
        FramePipelineCoordinator(LPDIRECT3DDEVICE9 dev);
        ~FramePipelineCoordinator() = default;

        // Disallow copy/move
        FramePipelineCoordinator(const FramePipelineCoordinator&) = delete;
        FramePipelineCoordinator& operator=(const FramePipelineCoordinator&) = delete;

        /**
         * @brief Begins the frame, resetting allocators.
         */
        void BeginFrame() noexcept;

        /**
         * @brief Collects geometry and instances (terrain, actors, speedtree, ui).
         */
        void PhaseCollect() noexcept;

        /**
         * @brief Sorts the RenderQueue using a 64-bit Radix Sort.
         */
        void PhaseSort() noexcept;

        /**
         * @brief Dispatches passes sequentially.
         */
        void PhaseDispatch() noexcept;

        /**
         * @brief Ends the frame, gathers statistics, and presents.
         */
        void EndFrame() noexcept;

        // Public accessors for testing or DI
        LinearFrameAllocator& GetAllocator() noexcept { return m_allocator; }
        DynamicRingVertexBuffer& GetRingBuffer() noexcept { return m_ringBuffer; }
        RenderQueue& GetRenderQueue() noexcept { return m_renderQueue; }
        FrameStatisticsTracker& GetStatsTracker() noexcept { return m_statsTracker; }

    private:
        LPDIRECT3DDEVICE9 m_device;
        LinearFrameAllocator m_allocator;
        DynamicRingVertexBuffer m_ringBuffer;
        RenderQueue m_renderQueue;
        FrameStatisticsTracker m_statsTracker;

        OpaquePassDispatcher m_opaquePass;
        AlphaTestPassDispatcher m_alphaTestPass;
        AlphaBlendPassDispatcher m_alphaBlendPass;
        AdditivePassDispatcher m_additivePass;
        DepthPrepassDispatcher m_depthPrepass;
        UIPassDispatcher m_uiPass;
    };
} // namespace EterLib::Render

