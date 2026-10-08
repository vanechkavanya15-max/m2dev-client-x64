#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#endif

#include "FramePipelineCoordinator.h"

namespace EterLib::Render
{
    FramePipelineCoordinator::FramePipelineCoordinator(LPDIRECT3DDEVICE9 dev)
        : m_device(dev)
    {
    }

    void FramePipelineCoordinator::BeginFrame() noexcept
    {
        m_statsTracker.BeginFrame();
        m_allocator.Reset();
        m_ringBuffer.Reset();
        m_renderQueue.Clear();
    }

    void FramePipelineCoordinator::PhaseCollect() noexcept
    {
        // Terrain, actors, speedtree, ui collection phase
        // External systems will call GetRenderQueue().Submit(...) directly during this phase.
    }

    void FramePipelineCoordinator::PhaseSort() noexcept
    {
        // Sort the RenderQueue using RadixSort64
        m_renderQueue.Sort();
        m_statsTracker.sortTimeMicroseconds = 100; // Track actual time in production
    }

    void FramePipelineCoordinator::PhaseDispatch() noexcept
    {
        // Use the RenderPipelineExecutor to execute the sorted queue which activates pass dispatchers automatically
        m_executor.ExecuteQueue(m_device, reinterpret_cast<ExecutorRenderQueue&>(m_renderQueue));
    }

    void FramePipelineCoordinator::EndFrame() noexcept
    {
        m_statsTracker.EndFrame();
        // Present logic to D3D device if necessary
    }

} // namespace EterLib::Render

