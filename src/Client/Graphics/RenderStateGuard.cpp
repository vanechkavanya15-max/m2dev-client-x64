#include "EterLib/StdAfx.h"
#include "EterLib/StateManager.h"
#include "Client/Graphics/RenderStateGuard.h"

namespace Client::Graphics
{

    void StateCache::Set(uint64_t key, uint32_t value)
    {
        std::unique_lock lock(m_mutex);
        m_cache[key] = value;
    }

    std::optional<uint32_t> StateCache::Get(uint64_t key) const
    {
        std::shared_lock lock(m_mutex);
        if (auto it = m_cache.find(key); it != m_cache.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    void StateCache::Invalidate(uint64_t key)
    {
        std::unique_lock lock(m_mutex);
        m_cache.erase(key);
    }

    void StateCache::Clear()
    {
        std::unique_lock lock(m_mutex);
        m_cache.clear();
    }


    // ---------------- AlphaBlendGuard ----------------
    AlphaBlendGuard::AlphaBlendGuard(bool enable, uint32_t srcBlend, uint32_t destBlend)
    {
        auto& cache = StateCache::GetInstance();

        // 1. D3DRS_ALPHABLENDENABLE
        uint64_t enableKey = StateCache::MakeKey_RenderState(D3DRS_ALPHABLENDENABLE);
        auto optEnable = cache.Get(enableKey);
        if (!optEnable || *optEnable != static_cast<uint32_t>(enable))
        {
            DWORD currentEnable = 0;
            // Get original state
            STATEMANAGER.GetRenderState(D3DRS_ALPHABLENDENABLE, &currentEnable);
            m_savedEnable = static_cast<uint32_t>(currentEnable);
            m_savedEnableStateValid = true;

            // Set new state
            STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, enable ? TRUE : FALSE);
            cache.Set(enableKey, enable ? TRUE : FALSE);
        }

        // 2. D3DRS_SRCBLEND
        uint64_t srcBlendKey = StateCache::MakeKey_RenderState(D3DRS_SRCBLEND);
        auto optSrcBlend = cache.Get(srcBlendKey);
        if (!optSrcBlend || *optSrcBlend != srcBlend)
        {
            DWORD currentSrcBlend = 0;
            STATEMANAGER.GetRenderState(D3DRS_SRCBLEND, &currentSrcBlend);
            m_savedSrcBlend = static_cast<uint32_t>(currentSrcBlend);
            m_savedSrcBlendStateValid = true;

            STATEMANAGER.SetRenderState(D3DRS_SRCBLEND, srcBlend);
            cache.Set(srcBlendKey, srcBlend);
        }

        // 3. D3DRS_DESTBLEND
        uint64_t destBlendKey = StateCache::MakeKey_RenderState(D3DRS_DESTBLEND);
        auto optDestBlend = cache.Get(destBlendKey);
        if (!optDestBlend || *optDestBlend != destBlend)
        {
            DWORD currentDestBlend = 0;
            STATEMANAGER.GetRenderState(D3DRS_DESTBLEND, &currentDestBlend);
            m_savedDestBlend = static_cast<uint32_t>(currentDestBlend);
            m_savedDestBlendStateValid = true;

            STATEMANAGER.SetRenderState(D3DRS_DESTBLEND, destBlend);
            cache.Set(destBlendKey, destBlend);
        }
    }

    AlphaBlendGuard::~AlphaBlendGuard()
    {
        auto& cache = StateCache::GetInstance();

        if (m_savedDestBlendStateValid)
        {
            STATEMANAGER.SetRenderState(D3DRS_DESTBLEND, m_savedDestBlend);
            cache.Set(StateCache::MakeKey_RenderState(D3DRS_DESTBLEND), m_savedDestBlend);
        }

        if (m_savedSrcBlendStateValid)
        {
            STATEMANAGER.SetRenderState(D3DRS_SRCBLEND, m_savedSrcBlend);
            cache.Set(StateCache::MakeKey_RenderState(D3DRS_SRCBLEND), m_savedSrcBlend);
        }

        if (m_savedEnableStateValid)
        {
            STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, m_savedEnable);
            cache.Set(StateCache::MakeKey_RenderState(D3DRS_ALPHABLENDENABLE), m_savedEnable);
        }
    }


    // ---------------- TextureFactorGuard ----------------
    TextureFactorGuard::TextureFactorGuard(uint32_t color)
    {
        auto& cache = StateCache::GetInstance();
        uint64_t colorKey = StateCache::MakeKey_RenderState(D3DRS_TEXTUREFACTOR);

        auto optColor = cache.Get(colorKey);
        if (!optColor || *optColor != color)
        {
            DWORD currentColor = 0;
            STATEMANAGER.GetRenderState(D3DRS_TEXTUREFACTOR, &currentColor);
            m_savedColor = static_cast<uint32_t>(currentColor);
            m_savedColorStateValid = true;

            STATEMANAGER.SetRenderState(D3DRS_TEXTUREFACTOR, color);
            cache.Set(colorKey, color);
        }
    }

    TextureFactorGuard::~TextureFactorGuard()
    {
        if (m_savedColorStateValid)
        {
            STATEMANAGER.SetRenderState(D3DRS_TEXTUREFACTOR, m_savedColor);
            StateCache::GetInstance().Set(StateCache::MakeKey_RenderState(D3DRS_TEXTUREFACTOR), m_savedColor);
        }
    }


    // ---------------- ZBufferGuard ----------------
    ZBufferGuard::ZBufferGuard(bool zEnable, bool zWriteEnable)
    {
        auto& cache = StateCache::GetInstance();

        // 1. D3DRS_ZENABLE
        uint64_t zEnableKey = StateCache::MakeKey_RenderState(D3DRS_ZENABLE);
        auto optZEnable = cache.Get(zEnableKey);
        if (!optZEnable || *optZEnable != static_cast<uint32_t>(zEnable))
        {
            DWORD currentZEnable = 0;
            STATEMANAGER.GetRenderState(D3DRS_ZENABLE, &currentZEnable);
            m_savedZEnable = static_cast<uint32_t>(currentZEnable);
            m_savedZEnableStateValid = true;

            STATEMANAGER.SetRenderState(D3DRS_ZENABLE, zEnable ? TRUE : FALSE);
            cache.Set(zEnableKey, zEnable ? TRUE : FALSE);
        }

        // 2. D3DRS_ZWRITEENABLE
        uint64_t zWriteEnableKey = StateCache::MakeKey_RenderState(D3DRS_ZWRITEENABLE);
        auto optZWriteEnable = cache.Get(zWriteEnableKey);
        if (!optZWriteEnable || *optZWriteEnable != static_cast<uint32_t>(zWriteEnable))
        {
            DWORD currentZWriteEnable = 0;
            STATEMANAGER.GetRenderState(D3DRS_ZWRITEENABLE, &currentZWriteEnable);
            m_savedZWriteEnable = static_cast<uint32_t>(currentZWriteEnable);
            m_savedZWriteEnableStateValid = true;

            STATEMANAGER.SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable ? TRUE : FALSE);
            cache.Set(zWriteEnableKey, zWriteEnable ? TRUE : FALSE);
        }
    }

    ZBufferGuard::~ZBufferGuard()
    {
        auto& cache = StateCache::GetInstance();

        if (m_savedZWriteEnableStateValid)
        {
            STATEMANAGER.SetRenderState(D3DRS_ZWRITEENABLE, m_savedZWriteEnable);
            cache.Set(StateCache::MakeKey_RenderState(D3DRS_ZWRITEENABLE), m_savedZWriteEnable);
        }

        if (m_savedZEnableStateValid)
        {
            STATEMANAGER.SetRenderState(D3DRS_ZENABLE, m_savedZEnable);
            cache.Set(StateCache::MakeKey_RenderState(D3DRS_ZENABLE), m_savedZEnable);
        }
    }


    // ---------------- SamplerStateGuard ----------------
    SamplerStateGuard::SamplerStateGuard(uint32_t stage, uint32_t type, uint32_t value)
        : m_stage(stage), m_type(type)
    {
        auto& cache = StateCache::GetInstance();
        uint64_t key = StateCache::MakeKey_SamplerState(stage, type);

        auto optValue = cache.Get(key);
        if (!optValue || *optValue != value)
        {
            DWORD currentValue = 0;
            STATEMANAGER.GetSamplerState(stage, static_cast<D3DSAMPLERSTATETYPE>(type), &currentValue);
            m_savedValue = static_cast<uint32_t>(currentValue);
            m_savedValueValid = true;

            STATEMANAGER.SetSamplerState(stage, static_cast<D3DSAMPLERSTATETYPE>(type), value);
            cache.Set(key, value);
        }
    }

    SamplerStateGuard::~SamplerStateGuard()
    {
        if (m_savedValueValid)
        {
            STATEMANAGER.SetSamplerState(m_stage, static_cast<D3DSAMPLERSTATETYPE>(m_type), m_savedValue);
            StateCache::GetInstance().Set(StateCache::MakeKey_SamplerState(m_stage, m_type), m_savedValue);
        }
    }

} // namespace Client::Graphics
