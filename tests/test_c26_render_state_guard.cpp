#include <iostream>
#include <cassert>
#include <unordered_map>
#include <vector>

// Dummy d3d9 types and STATEMANAGER mock
typedef unsigned int DWORD;
typedef int BOOL;
#define TRUE 1
#define FALSE 0

enum D3DRENDERSTATETYPE {
    D3DRS_ZENABLE = 7,
    D3DRS_ZWRITEENABLE = 14,
    D3DRS_ALPHABLENDENABLE = 27,
    D3DRS_SRCBLEND = 19,
    D3DRS_DESTBLEND = 20,
    D3DRS_TEXTUREFACTOR = 60,
};

enum D3DSAMPLERSTATETYPE {
    D3DSAMP_MAGFILTER = 5,
    D3DSAMP_MINFILTER = 6,
    D3DSAMP_MIPFILTER = 7,
};

struct MockStateManager {
    std::unordered_map<DWORD, DWORD> renderStates;
    std::unordered_map<DWORD, std::unordered_map<DWORD, DWORD>> samplerStates;
    int setRenderStateCalls = 0;
    int getRenderStateCalls = 0;
    int setSamplerStateCalls = 0;
    int getSamplerStateCalls = 0;

    void SetRenderState(D3DRENDERSTATETYPE Type, DWORD Value) {
        renderStates[Type] = Value;
        setRenderStateCalls++;
    }

    void GetRenderState(D3DRENDERSTATETYPE Type, DWORD* pdwValue) {
        *pdwValue = renderStates[Type];
        getRenderStateCalls++;
    }

    void SetSamplerState(DWORD dwStage, D3DSAMPLERSTATETYPE Type, DWORD dwValue) {
        samplerStates[dwStage][Type] = dwValue;
        setSamplerStateCalls++;
    }

    void GetSamplerState(DWORD dwStage, D3DSAMPLERSTATETYPE Type, DWORD* pdwValue) {
        *pdwValue = samplerStates[dwStage][Type];
        getSamplerStateCalls++;
    }

    void ResetCounters() {
        setRenderStateCalls = 0;
        getRenderStateCalls = 0;
        setSamplerStateCalls = 0;
        getSamplerStateCalls = 0;
    }
};

MockStateManager STATEMANAGER;

// Include the headers for our mock environment
#include "../src/Client/Graphics/RenderStateGuard.h"

// Re-implement RenderStateGuard.cpp logic here to use our MockStateManager instead of the real one.
namespace Client::Graphics
{
    void StateCache::Set(uint64_t key, uint32_t value) {
        std::unique_lock lock(m_mutex);
        m_cache[key] = value;
    }
    std::optional<uint32_t> StateCache::Get(uint64_t key) const {
        std::shared_lock lock(m_mutex);
        if (auto it = m_cache.find(key); it != m_cache.end()) return it->second;
        return std::nullopt;
    }
    void StateCache::Invalidate(uint64_t key) {
        std::unique_lock lock(m_mutex);
        m_cache.erase(key);
    }
    void StateCache::Clear() {
        std::unique_lock lock(m_mutex);
        m_cache.clear();
    }

    AlphaBlendGuard::AlphaBlendGuard(bool enable, uint32_t srcBlend, uint32_t destBlend) {
        auto& cache = StateCache::GetInstance();
        uint64_t enableKey = StateCache::MakeKey_RenderState(D3DRS_ALPHABLENDENABLE);
        auto optEnable = cache.Get(enableKey);
        if (!optEnable || *optEnable != static_cast<uint32_t>(enable)) {
            uint32_t currentEnable = 0;
            STATEMANAGER.GetRenderState(D3DRS_ALPHABLENDENABLE, &currentEnable);
            m_savedEnable = currentEnable;
            m_savedEnableStateValid = true;
            STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, enable ? TRUE : FALSE);
            cache.Set(enableKey, enable ? TRUE : FALSE);
        }

        uint64_t srcBlendKey = StateCache::MakeKey_RenderState(D3DRS_SRCBLEND);
        auto optSrcBlend = cache.Get(srcBlendKey);
        if (!optSrcBlend || *optSrcBlend != srcBlend) {
            uint32_t currentSrcBlend = 0;
            STATEMANAGER.GetRenderState(D3DRS_SRCBLEND, &currentSrcBlend);
            m_savedSrcBlend = currentSrcBlend;
            m_savedSrcBlendStateValid = true;
            STATEMANAGER.SetRenderState(D3DRS_SRCBLEND, srcBlend);
            cache.Set(srcBlendKey, srcBlend);
        }

        uint64_t destBlendKey = StateCache::MakeKey_RenderState(D3DRS_DESTBLEND);
        auto optDestBlend = cache.Get(destBlendKey);
        if (!optDestBlend || *optDestBlend != destBlend) {
            uint32_t currentDestBlend = 0;
            STATEMANAGER.GetRenderState(D3DRS_DESTBLEND, &currentDestBlend);
            m_savedDestBlend = currentDestBlend;
            m_savedDestBlendStateValid = true;
            STATEMANAGER.SetRenderState(D3DRS_DESTBLEND, destBlend);
            cache.Set(destBlendKey, destBlend);
        }
    }
    AlphaBlendGuard::~AlphaBlendGuard() {
        auto& cache = StateCache::GetInstance();
        if (m_savedDestBlendStateValid) {
            STATEMANAGER.SetRenderState(D3DRS_DESTBLEND, m_savedDestBlend);
            cache.Set(StateCache::MakeKey_RenderState(D3DRS_DESTBLEND), m_savedDestBlend);
        }
        if (m_savedSrcBlendStateValid) {
            STATEMANAGER.SetRenderState(D3DRS_SRCBLEND, m_savedSrcBlend);
            cache.Set(StateCache::MakeKey_RenderState(D3DRS_SRCBLEND), m_savedSrcBlend);
        }
        if (m_savedEnableStateValid) {
            STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, m_savedEnable);
            cache.Set(StateCache::MakeKey_RenderState(D3DRS_ALPHABLENDENABLE), m_savedEnable);
        }
    }

    TextureFactorGuard::TextureFactorGuard(uint32_t color) {
        auto& cache = StateCache::GetInstance();
        uint64_t colorKey = StateCache::MakeKey_RenderState(D3DRS_TEXTUREFACTOR);
        auto optColor = cache.Get(colorKey);
        if (!optColor || *optColor != color) {
            uint32_t currentColor = 0;
            STATEMANAGER.GetRenderState(D3DRS_TEXTUREFACTOR, &currentColor);
            m_savedColor = currentColor;
            m_savedColorStateValid = true;
            STATEMANAGER.SetRenderState(D3DRS_TEXTUREFACTOR, color);
            cache.Set(colorKey, color);
        }
    }
    TextureFactorGuard::~TextureFactorGuard() {
        if (m_savedColorStateValid) {
            STATEMANAGER.SetRenderState(D3DRS_TEXTUREFACTOR, m_savedColor);
            StateCache::GetInstance().Set(StateCache::MakeKey_RenderState(D3DRS_TEXTUREFACTOR), m_savedColor);
        }
    }

    ZBufferGuard::ZBufferGuard(bool zEnable, bool zWriteEnable) {
        auto& cache = StateCache::GetInstance();
        uint64_t zEnableKey = StateCache::MakeKey_RenderState(D3DRS_ZENABLE);
        auto optZEnable = cache.Get(zEnableKey);
        if (!optZEnable || *optZEnable != static_cast<uint32_t>(zEnable)) {
            uint32_t currentZEnable = 0;
            STATEMANAGER.GetRenderState(D3DRS_ZENABLE, &currentZEnable);
            m_savedZEnable = currentZEnable;
            m_savedZEnableStateValid = true;
            STATEMANAGER.SetRenderState(D3DRS_ZENABLE, zEnable ? TRUE : FALSE);
            cache.Set(zEnableKey, zEnable ? TRUE : FALSE);
        }

        uint64_t zWriteEnableKey = StateCache::MakeKey_RenderState(D3DRS_ZWRITEENABLE);
        auto optZWriteEnable = cache.Get(zWriteEnableKey);
        if (!optZWriteEnable || *optZWriteEnable != static_cast<uint32_t>(zWriteEnable)) {
            uint32_t currentZWriteEnable = 0;
            STATEMANAGER.GetRenderState(D3DRS_ZWRITEENABLE, &currentZWriteEnable);
            m_savedZWriteEnable = currentZWriteEnable;
            m_savedZWriteEnableStateValid = true;
            STATEMANAGER.SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable ? TRUE : FALSE);
            cache.Set(zWriteEnableKey, zWriteEnable ? TRUE : FALSE);
        }
    }
    ZBufferGuard::~ZBufferGuard() {
        auto& cache = StateCache::GetInstance();
        if (m_savedZWriteEnableStateValid) {
            STATEMANAGER.SetRenderState(D3DRS_ZWRITEENABLE, m_savedZWriteEnable);
            cache.Set(StateCache::MakeKey_RenderState(D3DRS_ZWRITEENABLE), m_savedZWriteEnable);
        }
        if (m_savedZEnableStateValid) {
            STATEMANAGER.SetRenderState(D3DRS_ZENABLE, m_savedZEnable);
            cache.Set(StateCache::MakeKey_RenderState(D3DRS_ZENABLE), m_savedZEnable);
        }
    }

    SamplerStateGuard::SamplerStateGuard(uint32_t stage, uint32_t type, uint32_t value) : m_stage(stage), m_type(type) {
        auto& cache = StateCache::GetInstance();
        uint64_t key = StateCache::MakeKey_SamplerState(stage, type);
        auto optValue = cache.Get(key);
        if (!optValue || *optValue != value) {
            uint32_t currentValue = 0;
            STATEMANAGER.GetSamplerState(stage, static_cast<D3DSAMPLERSTATETYPE>(type), &currentValue);
            m_savedValue = currentValue;
            m_savedValueValid = true;
            STATEMANAGER.SetSamplerState(stage, static_cast<D3DSAMPLERSTATETYPE>(type), value);
            cache.Set(key, value);
        }
    }
    SamplerStateGuard::~SamplerStateGuard() {
        if (m_savedValueValid) {
            STATEMANAGER.SetSamplerState(m_stage, static_cast<D3DSAMPLERSTATETYPE>(m_type), m_savedValue);
            StateCache::GetInstance().Set(StateCache::MakeKey_SamplerState(m_stage, m_type), m_savedValue);
        }
    }
}

void TestAlphaBlendGuard()
{
    using namespace Client::Graphics;
    StateCache::GetInstance().Clear();
    STATEMANAGER.ResetCounters();

    STATEMANAGER.renderStates[D3DRS_ALPHABLENDENABLE] = 0;
    STATEMANAGER.renderStates[D3DRS_SRCBLEND] = 1;
    STATEMANAGER.renderStates[D3DRS_DESTBLEND] = 2;

    {
        AlphaBlendGuard guard(true, 5, 6);
        assert(STATEMANAGER.renderStates[D3DRS_ALPHABLENDENABLE] == 1);
        assert(STATEMANAGER.renderStates[D3DRS_SRCBLEND] == 5);
        assert(STATEMANAGER.renderStates[D3DRS_DESTBLEND] == 6);
        assert(STATEMANAGER.setRenderStateCalls == 3);
        assert(STATEMANAGER.getRenderStateCalls == 3);

        STATEMANAGER.ResetCounters();

        // Testing deduplication logic (cache hit)
        {
            AlphaBlendGuard guard2(true, 5, 6);
            assert(STATEMANAGER.setRenderStateCalls == 0);
            assert(STATEMANAGER.getRenderStateCalls == 0);
        }
    }

    // Checking restoration
    assert(STATEMANAGER.renderStates[D3DRS_ALPHABLENDENABLE] == 0);
    assert(STATEMANAGER.renderStates[D3DRS_SRCBLEND] == 1);
    assert(STATEMANAGER.renderStates[D3DRS_DESTBLEND] == 2);
    
    std::cout << "TestAlphaBlendGuard passed." << std::endl;
}

void TestTextureFactorGuard()
{
    using namespace Client::Graphics;
    StateCache::GetInstance().Clear();
    STATEMANAGER.ResetCounters();

    STATEMANAGER.renderStates[D3DRS_TEXTUREFACTOR] = 0xFFFFFFFF;

    {
        TextureFactorGuard guard(0x00FF00FF);
        assert(STATEMANAGER.renderStates[D3DRS_TEXTUREFACTOR] == 0x00FF00FF);
        assert(STATEMANAGER.setRenderStateCalls == 1);
        assert(STATEMANAGER.getRenderStateCalls == 1);

        STATEMANAGER.ResetCounters();

        {
            TextureFactorGuard guard2(0x00FF00FF);
            assert(STATEMANAGER.setRenderStateCalls == 0);
            assert(STATEMANAGER.getRenderStateCalls == 0);
        }
    }

    assert(STATEMANAGER.renderStates[D3DRS_TEXTUREFACTOR] == 0xFFFFFFFF);
    std::cout << "TestTextureFactorGuard passed." << std::endl;
}

void TestZBufferGuard()
{
    using namespace Client::Graphics;
    StateCache::GetInstance().Clear();
    STATEMANAGER.ResetCounters();

    STATEMANAGER.renderStates[D3DRS_ZENABLE] = 1;
    STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] = 1;

    {
        ZBufferGuard guard(false, false);
        assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == 0);
        assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == 0);
        assert(STATEMANAGER.setRenderStateCalls == 2);
        assert(STATEMANAGER.getRenderStateCalls == 2);

        STATEMANAGER.ResetCounters();

        {
            ZBufferGuard guard2(false, false);
            assert(STATEMANAGER.setRenderStateCalls == 0);
            assert(STATEMANAGER.getRenderStateCalls == 0);
        }
    }

    assert(STATEMANAGER.renderStates[D3DRS_ZENABLE] == 1);
    assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == 1);
    std::cout << "TestZBufferGuard passed." << std::endl;
}

void TestSamplerStateGuard()
{
    using namespace Client::Graphics;
    StateCache::GetInstance().Clear();
    STATEMANAGER.ResetCounters();

    STATEMANAGER.samplerStates[0][D3DSAMP_MAGFILTER] = 1;

    {
        SamplerStateGuard guard(0, D3DSAMP_MAGFILTER, 2);
        assert(STATEMANAGER.samplerStates[0][D3DSAMP_MAGFILTER] == 2);
        assert(STATEMANAGER.setSamplerStateCalls == 1);
        assert(STATEMANAGER.getSamplerStateCalls == 1);

        STATEMANAGER.ResetCounters();

        {
            SamplerStateGuard guard2(0, D3DSAMP_MAGFILTER, 2);
            assert(STATEMANAGER.setSamplerStateCalls == 0);
            assert(STATEMANAGER.getSamplerStateCalls == 0);
        }
    }

    assert(STATEMANAGER.samplerStates[0][D3DSAMP_MAGFILTER] == 1);
    std::cout << "TestSamplerStateGuard passed." << std::endl;
}

int main()
{
    TestAlphaBlendGuard();
    TestTextureFactorGuard();
    TestZBufferGuard();
    TestSamplerStateGuard();
    std::cout << "All C++26 RAII RenderStateGuard tests passed successfully!" << std::endl;
    return 0;
}
