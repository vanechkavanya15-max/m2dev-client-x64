#pragma once

#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <optional>

namespace Client::Graphics
{

    class StateCache
    {
    public:
        static StateCache& GetInstance()
        {
            static StateCache instance;
            return instance;
        }

        // Keys are constructed to uniquely identify a D3D state (RenderState, TextureStageState, SamplerState)
        // Upper 32 bits: Type (e.g. 0 for RenderState, 1 for TextureStageState, 2 for SamplerState, stage for the latter two)
        // Lower 32 bits: State enum (e.g. D3DRS_ALPHABLENDENABLE)
        void Set(uint64_t key, uint32_t value);
        std::optional<uint32_t> Get(uint64_t key) const;
        void Invalidate(uint64_t key);
        void Clear();

        static constexpr uint64_t MakeKey_RenderState(uint32_t type)
        {
            return (0ULL << 62) | type;
        }

        static constexpr uint64_t MakeKey_TextureStageState(uint32_t stage, uint32_t type)
        {
            return (1ULL << 62) | (static_cast<uint64_t>(stage) << 32) | type;
        }

        static constexpr uint64_t MakeKey_SamplerState(uint32_t stage, uint32_t type)
        {
            return (2ULL << 62) | (static_cast<uint64_t>(stage) << 32) | type;
        }

    private:
        StateCache() = default;
        ~StateCache() = default;

        StateCache(const StateCache&) = delete;
        StateCache& operator=(const StateCache&) = delete;

        mutable std::shared_mutex m_mutex;
        std::unordered_map<uint64_t, uint32_t> m_cache;
    };

    class AlphaBlendGuard
    {
    public:
        AlphaBlendGuard(bool enable, uint32_t srcBlend, uint32_t destBlend);
        ~AlphaBlendGuard();

    private:
        bool m_savedEnableStateValid{ false };
        uint32_t m_savedEnable{ 0 };

        bool m_savedSrcBlendStateValid{ false };
        uint32_t m_savedSrcBlend{ 0 };

        bool m_savedDestBlendStateValid{ false };
        uint32_t m_savedDestBlend{ 0 };
    };

    class TextureFactorGuard
    {
    public:
        explicit TextureFactorGuard(uint32_t color);
        ~TextureFactorGuard();

    private:
        bool m_savedColorStateValid{ false };
        uint32_t m_savedColor{ 0 };
    };

    class ZBufferGuard
    {
    public:
        ZBufferGuard(bool zEnable, bool zWriteEnable);
        ~ZBufferGuard();

    private:
        bool m_savedZEnableStateValid{ false };
        uint32_t m_savedZEnable{ 0 };

        bool m_savedZWriteEnableStateValid{ false };
        uint32_t m_savedZWriteEnable{ 0 };
    };

    class SamplerStateGuard
    {
    public:
        SamplerStateGuard(uint32_t stage, uint32_t type, uint32_t value);
        ~SamplerStateGuard();

    private:
        uint32_t m_stage;
        uint32_t m_type;
        bool m_savedValueValid{ false };
        uint32_t m_savedValue{ 0 };
    };

} // namespace Client::Graphics
