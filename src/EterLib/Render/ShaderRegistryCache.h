#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#endif

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <functional>

namespace EterLib::Render
{
    struct ShaderBinding
    {
        LPDIRECT3DVERTEXSHADER9 vs;
        LPDIRECT3DPIXELSHADER9 ps;
        DWORD fvf;

        bool operator==(const ShaderBinding& other) const noexcept
        {
            return vs == other.vs && ps == other.ps && fvf == other.fvf;
        }
    };
}

namespace std
{
    template <>
    struct hash<EterLib::Render::ShaderBinding>
    {
        std::size_t operator()(const EterLib::Render::ShaderBinding& k) const noexcept
        {
            return (std::hash<void*>()((void*)k.vs)) ^ 
                   (std::hash<void*>()((void*)k.ps) << 1) ^ 
                   (std::hash<DWORD>()(k.fvf) << 2);
        }
    };
}

namespace EterLib::Render
{
    class ShaderRegistryCache
    {
    public:
        ShaderRegistryCache() noexcept = default;
        ~ShaderRegistryCache() noexcept = default;

        // Disallow copying and moving
        ShaderRegistryCache(const ShaderRegistryCache&) = delete;
        ShaderRegistryCache& operator=(const ShaderRegistryCache&) = delete;
        ShaderRegistryCache(ShaderRegistryCache&&) = delete;
        ShaderRegistryCache& operator=(ShaderRegistryCache&&) = delete;

        uint16_t GetOrCreateShaderPairId(LPDIRECT3DVERTEXSHADER9 vs, LPDIRECT3DPIXELSHADER9 ps, DWORD fvf = 0) noexcept;
        ShaderBinding GetBindingById(uint16_t id) const noexcept;
        void Clear() noexcept;

    private:
        static constexpr uint16_t MAX_ID = 4095;

        std::unordered_map<ShaderBinding, uint16_t> m_bindingToId;
        std::vector<ShaderBinding> m_idToBinding;
    };
}

