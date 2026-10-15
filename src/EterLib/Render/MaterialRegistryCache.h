#pragma once

#ifndef __linux__
#include "../StdAfx.h"
#else
// Ensure we have types for Linux test mock
struct D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
};
struct D3DMATERIAL9 {
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Ambient;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Emissive;
    float Power;
};
#endif

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <cstring>

namespace EterLib::Render
{
    struct MaterialHasher
    {
        std::size_t operator()(const D3DMATERIAL9& mat) const noexcept
        {
            // Proste hashowanie bitowe.
            const uint32_t* p = reinterpret_cast<const uint32_t*>(&mat);
            std::size_t hash = 0;
            constexpr std::size_t words = sizeof(D3DMATERIAL9) / sizeof(uint32_t);
            for (std::size_t i = 0; i < words; ++i)
            {
                hash ^= p[i] + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };

    struct MaterialEqual
    {
        bool operator()(const D3DMATERIAL9& lhs, const D3DMATERIAL9& rhs) const noexcept
        {
            return std::memcmp(&lhs, &rhs, sizeof(D3DMATERIAL9)) == 0;
        }
    };

    class MaterialRegistryCache
    {
    public:
        MaterialRegistryCache() = default;
        ~MaterialRegistryCache() = default;

        MaterialRegistryCache(const MaterialRegistryCache&) = delete;
        MaterialRegistryCache& operator=(const MaterialRegistryCache&) = delete;
        MaterialRegistryCache(MaterialRegistryCache&&) = delete;
        MaterialRegistryCache& operator=(MaterialRegistryCache&&) = delete;

        uint8_t GetOrCreateMaterialId(const D3DMATERIAL9& mat) noexcept;
        const D3DMATERIAL9& GetMaterialById(uint8_t id) const noexcept;
        void Clear() noexcept;

    private:
        std::unordered_map<D3DMATERIAL9, uint8_t, MaterialHasher, MaterialEqual> m_materialToId;
        std::vector<D3DMATERIAL9> m_idToMaterial;
    };
} // namespace EterLib::Render

