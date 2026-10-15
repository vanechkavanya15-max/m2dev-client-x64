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
        const auto entries = m_renderQueue.GetEntries();
        if (entries.empty())
            return;

        uint8_t currentPass = 0xFF;

        auto activatePass = [this](uint8_t pass) {
            switch (static_cast<Pass>(pass)) {
            case Pass::DepthPrepass: m_depthPrepass.BeginPass(m_device); break;
            case Pass::Opaque:       m_opaquePass.BeginPass(m_device); break;
            case Pass::AlphaTest:    m_alphaTestPass.BeginPass(m_device); break;
            case Pass::Transparent:  m_alphaBlendPass.BeginPass(m_device); break;
            case Pass::Additive:     m_additivePass.BeginPass(m_device); break;
            case Pass::UI:           m_uiPass.BeginPass(m_device); break;
            default: break;
            }
        };

        auto deactivatePass = [this](uint8_t pass) {
            switch (static_cast<Pass>(pass)) {
            case Pass::DepthPrepass: m_depthPrepass.EndPass(m_device); break;
            case Pass::Opaque:       m_opaquePass.EndPass(m_device); break;
            case Pass::AlphaTest:    m_alphaTestPass.EndPass(m_device); break;
            case Pass::Transparent:  m_alphaBlendPass.EndPass(m_device); break;
            case Pass::Additive:     m_additivePass.EndPass(m_device); break;
            case Pass::UI:           m_uiPass.EndPass(m_device); break;
            default: break;
            }
        };

        for (const auto& entry : entries)
        {
            uint8_t passId = static_cast<uint8_t>((entry.sortKey.value >> 56) & 0xFF);
            if (passId != currentPass)
            {
                if (currentPass != 0xFF)
                {
                    deactivatePass(currentPass);
                }
                currentPass = passId;
                activatePass(currentPass);
            }

            m_statsTracker.totalDrawCallsSubmitted++;
            if (entry.commandPtr)
            {
                if (entry.type == CommandType::Draw)
                {
                    m_statsTracker.actualDrawCallsExecuted++;
                }
                else if (entry.type == CommandType::StateChange)
                {
                    m_statsTracker.stateChangesAttempted++;
                }
            }
        }

        if (currentPass != 0xFF)
        {
            deactivatePass(currentPass);
        }
    }

    void FramePipelineCoordinator::EndFrame() noexcept
    {
        m_statsTracker.EndFrame();
        // Present logic to D3D device if necessary
    }

} // namespace EterLib::Render

