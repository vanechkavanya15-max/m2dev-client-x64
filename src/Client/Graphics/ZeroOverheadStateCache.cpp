#include "Client/Graphics/ZeroOverheadStateCache.h"

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d9.h>
#endif

namespace Client::Graphics
{
    ZeroOverheadStateCache::ZeroOverheadStateCache() noexcept
    {
        ResetToDefaults();
    }

    void ZeroOverheadStateCache::BindDevice(IDirect3DDevice9* device) noexcept
    {
        m_device = device;
    }

    IDirect3DDevice9* ZeroOverheadStateCache::GetDevice() const noexcept
    {
        return m_device;
    }

    void ZeroOverheadStateCache::SetCallbacks(const StateDispatchCallbacks& callbacks) noexcept
    {
        m_callbacks = callbacks;
    }

    void ZeroOverheadStateCache::ClearCallbacks() noexcept
    {
        m_callbacks = {};
    }

    bool ZeroOverheadStateCache::SetRenderState(uint32_t stateType, uint32_t value) noexcept
    {
        if (stateType >= NUM_RENDER_STATES)
        {
            return false;
        }

        const size_t blockIdx = stateType / 64;
        const uint64_t bitMask = 1ULL << (stateType % 64);
        const bool isValid = (m_validRenderStates[blockIdx] & bitMask) != 0;

        // Jesli stan jest identyczny i wazny - natychmiastowy return false i inkrementacja m_filteredCalls
        if (isValid && m_renderStates[stateType] == value)
        {
            ++m_filteredCalls;
            return false;
        }

        // Zapisanie nowego stanu i oznaczenie jako wazny i brudny
        m_renderStates[stateType] = value;
        m_validRenderStates[blockIdx] |= bitMask;
        m_dirtyRenderStates[blockIdx] |= bitMask;
        ++m_forwardedCalls;

#if defined(_WIN32) || defined(_WIN64)
        if (m_device)
        {
            m_device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(stateType), value);
        }
#endif

        if (m_callbacks.onSetRenderState)
        {
            m_callbacks.onSetRenderState(stateType, value, m_callbacks.userData);
        }

        return true;
    }

    uint32_t ZeroOverheadStateCache::GetRenderState(uint32_t stateType) const noexcept
    {
        if (stateType >= NUM_RENDER_STATES)
        {
            return 0;
        }
        return m_renderStates[stateType];
    }

    void ZeroOverheadStateCache::PushRenderState(uint32_t stateType, uint32_t newValue) noexcept
    {
        if (stateType >= NUM_RENDER_STATES)
        {
            return;
        }

        // Zapisanie biezacej wartosci na bezalokacyjny stos
        m_renderStateStacks[stateType].Push(m_renderStates[stateType]);

        // Ustawienie nowej wartosci przez cache
        SetRenderState(stateType, newValue);
    }

    void ZeroOverheadStateCache::PopRenderState(uint32_t stateType) noexcept
    {
        if (stateType >= NUM_RENDER_STATES)
        {
            return;
        }

        uint32_t previousValue = 0;
        if (m_renderStateStacks[stateType].Pop(previousValue))
        {
            // Przywrocenie poprzedniej wartosci
            SetRenderState(stateType, previousValue);
        }
    }

    bool ZeroOverheadStateCache::SetTextureStageState(uint32_t stage, uint32_t stateType, uint32_t value) noexcept
    {
        if (stage >= NUM_TEXTURE_STAGES || stateType >= NUM_STAGE_STATES)
        {
            return false;
        }

        const uint64_t bitMask = 1ULL << stateType;
        const bool isValid = (m_validTextureStageStates[stage] & bitMask) != 0;

        if (isValid && m_textureStageStates[stage][stateType] == value)
        {
            ++m_filteredCalls;
            return false;
        }

        m_textureStageStates[stage][stateType] = value;
        m_validTextureStageStates[stage] |= bitMask;
        ++m_forwardedCalls;

#if defined(_WIN32) || defined(_WIN64)
        if (m_device)
        {
            m_device->SetTextureStageState(stage, static_cast<D3DTEXTURESTAGESTATETYPE>(stateType), value);
        }
#endif

        if (m_callbacks.onSetTextureStageState)
        {
            m_callbacks.onSetTextureStageState(stage, stateType, value, m_callbacks.userData);
        }

        return true;
    }

    uint32_t ZeroOverheadStateCache::GetTextureStageState(uint32_t stage, uint32_t stateType) const noexcept
    {
        if (stage >= NUM_TEXTURE_STAGES || stateType >= NUM_STAGE_STATES)
        {
            return 0;
        }
        return m_textureStageStates[stage][stateType];
    }

    bool ZeroOverheadStateCache::SetSamplerState(uint32_t stage, uint32_t stateType, uint32_t value) noexcept
    {
        if (stage >= NUM_TEXTURE_STAGES || stateType >= NUM_SAMPLER_STATES)
        {
            return false;
        }

        const uint16_t bitMask = static_cast<uint16_t>(1U << stateType);
        const bool isValid = (m_validSamplerStates[stage] & bitMask) != 0;

        if (isValid && m_samplerStates[stage][stateType] == value)
        {
            ++m_filteredCalls;
            return false;
        }

        m_samplerStates[stage][stateType] = value;
        m_validSamplerStates[stage] |= bitMask;
        ++m_forwardedCalls;

#if defined(_WIN32) || defined(_WIN64)
        if (m_device)
        {
            m_device->SetSamplerState(stage, static_cast<D3DSAMPLERSTATETYPE>(stateType), value);
        }
#endif

        if (m_callbacks.onSetSamplerState)
        {
            m_callbacks.onSetSamplerState(stage, stateType, value, m_callbacks.userData);
        }

        return true;
    }

    uint32_t ZeroOverheadStateCache::GetSamplerState(uint32_t stage, uint32_t stateType) const noexcept
    {
        if (stage >= NUM_TEXTURE_STAGES || stateType >= NUM_SAMPLER_STATES)
        {
            return 0;
        }
        return m_samplerStates[stage][stateType];
    }

    void ZeroOverheadStateCache::ResetToDefaults() noexcept
    {
        m_renderStates.fill(0);
        for (auto& stage : m_textureStageStates)
        {
            stage.fill(0);
        }
        for (auto& sampler : m_samplerStates)
        {
            sampler.fill(0);
        }

        m_dirtyRenderStates[0] = 0;
        m_dirtyRenderStates[1] = 0;
        m_dirtyRenderStates[2] = 0;
        m_dirtyRenderStates[3] = 0;

        m_validRenderStates[0] = ~0ULL;
        m_validRenderStates[1] = ~0ULL;
        m_validRenderStates[2] = ~0ULL;
        m_validRenderStates[3] = ~0ULL;

        m_validTextureStageStates.fill(~0ULL);
        m_validSamplerStates.fill(0xFFFF);

        for (auto& stack : m_renderStateStacks)
        {
            stack.Clear();
        }
    }

    void ZeroOverheadStateCache::InvalidateAll() noexcept
    {
        m_renderStates.fill(0xFFFFFFFF);
        for (auto& stage : m_textureStageStates)
        {
            stage.fill(0xFFFFFFFF);
        }
        for (auto& sampler : m_samplerStates)
        {
            sampler.fill(0xFFFFFFFF);
        }

        m_dirtyRenderStates[0] = ~0ULL;
        m_dirtyRenderStates[1] = ~0ULL;
        m_dirtyRenderStates[2] = ~0ULL;
        m_dirtyRenderStates[3] = ~0ULL;

        m_validRenderStates[0] = 0;
        m_validRenderStates[1] = 0;
        m_validRenderStates[2] = 0;
        m_validRenderStates[3] = 0;

        m_validTextureStageStates.fill(0);
        m_validSamplerStates.fill(0);

        for (auto& stack : m_renderStateStacks)
        {
            stack.Clear();
        }
    }

    uint64_t ZeroOverheadStateCache::GetDirtyBlock(size_t blockIndex) const noexcept
    {
        if (blockIndex < 4)
        {
            return m_dirtyRenderStates[blockIndex];
        }
        return 0;
    }

    bool ZeroOverheadStateCache::IsRenderStateDirty(uint32_t stateType) const noexcept
    {
        if (stateType >= NUM_RENDER_STATES)
        {
            return false;
        }
        const size_t blockIdx = stateType / 64;
        const uint64_t bitMask = 1ULL << (stateType % 64);
        return (m_dirtyRenderStates[blockIdx] & bitMask) != 0;
    }

    void ZeroOverheadStateCache::ClearDirtyRenderState(uint32_t stateType) noexcept
    {
        if (stateType < NUM_RENDER_STATES)
        {
            const size_t blockIdx = stateType / 64;
            const uint64_t bitMask = 1ULL << (stateType % 64);
            m_dirtyRenderStates[blockIdx] &= ~bitMask;
        }
    }

    void ZeroOverheadStateCache::ClearAllDirty() noexcept
    {
        m_dirtyRenderStates[0] = 0;
        m_dirtyRenderStates[1] = 0;
        m_dirtyRenderStates[2] = 0;
        m_dirtyRenderStates[3] = 0;
    }

    void ZeroOverheadStateCache::MarkRenderStateDirty(uint32_t stateType) noexcept
    {
        if (stateType < NUM_RENDER_STATES)
        {
            const size_t blockIdx = stateType / 64;
            const uint64_t bitMask = 1ULL << (stateType % 64);
            m_dirtyRenderStates[blockIdx] |= bitMask;
        }
    }

    void ZeroOverheadStateCache::MarkAllDirty() noexcept
    {
        m_dirtyRenderStates[0] = ~0ULL;
        m_dirtyRenderStates[1] = ~0ULL;
        m_dirtyRenderStates[2] = ~0ULL;
        m_dirtyRenderStates[3] = ~0ULL;
    }

    bool ZeroOverheadStateCache::HasAnyDirtyStates() const noexcept
    {
        return (m_dirtyRenderStates[0] | m_dirtyRenderStates[1] |
                m_dirtyRenderStates[2] | m_dirtyRenderStates[3]) != 0ULL;
    }

    uint64_t ZeroOverheadStateCache::GetFilteredCalls() const noexcept
    {
        return m_filteredCalls;
    }

    uint64_t ZeroOverheadStateCache::GetForwardedCalls() const noexcept
    {
        return m_forwardedCalls;
    }

    double ZeroOverheadStateCache::GetEfficiencyRatio() const noexcept
    {
        const uint64_t total = m_filteredCalls + m_forwardedCalls;
        if (total == 0)
        {
            return 0.0;
        }
        return static_cast<double>(m_filteredCalls) / static_cast<double>(total);
    }

    void ZeroOverheadStateCache::ResetTelemetry() noexcept
    {
        m_filteredCalls = 0;
        m_forwardedCalls = 0;
    }
}
