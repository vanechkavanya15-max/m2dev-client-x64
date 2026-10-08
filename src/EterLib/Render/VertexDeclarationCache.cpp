#include "VertexDeclarationCache.h"

namespace EterLib::Render
{
    VertexDeclarationCache::~VertexDeclarationCache()
    {
        Clear();
    }

    LPDIRECT3DVERTEXDECLARATION9 VertexDeclarationCache::GetOrCreate(LPDIRECT3DDEVICE9 dev, std::span<const D3DVERTEXELEMENT9> elements)
    {
        if (!dev || elements.empty())
            return nullptr;

        auto it = m_cache.find(elements);
        if (it != m_cache.end())
        {
            return it->second;
        }

        LPDIRECT3DVERTEXDECLARATION9 decl = nullptr;
        if (dev->CreateVertexDeclaration(elements.data(), &decl) == D3D_OK)
        {
            m_cache.emplace(std::vector<D3DVERTEXELEMENT9>(elements.begin(), elements.end()), decl);
            return decl;
        }

        return nullptr;
    }

    void VertexDeclarationCache::Clear() noexcept
    {
        for (auto& [key, decl] : m_cache)
        {
            if (decl)
            {
                decl->Release();
            }
        }
        m_cache.clear();
    }

    size_t VertexDeclarationCache::Count() const noexcept
    {
        return m_cache.size();
    }
}

