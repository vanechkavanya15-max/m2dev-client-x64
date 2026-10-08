#pragma once

#include "../StdAfx.h"
#include <vector>
#include <span>
#include <unordered_map>
#include <cstring>

namespace EterLib::Render
{
    class VertexDeclarationCache
    {
    public:
        VertexDeclarationCache() = default;
        ~VertexDeclarationCache();

        VertexDeclarationCache(const VertexDeclarationCache&) = delete;
        VertexDeclarationCache& operator=(const VertexDeclarationCache&) = delete;

        LPDIRECT3DVERTEXDECLARATION9 GetOrCreate(LPDIRECT3DDEVICE9 dev, std::span<const D3DVERTEXELEMENT9> elements);
        void Clear() noexcept;
        size_t Count() const noexcept;

    private:
        struct ElementsHasher
        {
            using is_transparent = void;

            std::size_t operator()(const std::vector<D3DVERTEXELEMENT9>& elements) const noexcept
            {
                return hash_bytes(reinterpret_cast<const unsigned char*>(elements.data()), elements.size() * sizeof(D3DVERTEXELEMENT9));
            }

            std::size_t operator()(std::span<const D3DVERTEXELEMENT9> elements) const noexcept
            {
                return hash_bytes(reinterpret_cast<const unsigned char*>(elements.data()), elements.size() * sizeof(D3DVERTEXELEMENT9));
            }

        private:
            std::size_t hash_bytes(const unsigned char* p, std::size_t numBytes) const noexcept
            {
                std::size_t hash = 14695981039346656037ull;
                for (std::size_t i = 0; i < numBytes; ++i)
                {
                    hash ^= p[i];
                    hash *= 1099511628211ull;
                }
                return hash;
            }
        };

        struct ElementsEqual
        {
            using is_transparent = void;

            bool operator()(const std::vector<D3DVERTEXELEMENT9>& lhs, const std::vector<D3DVERTEXELEMENT9>& rhs) const noexcept
            {
                return compare(lhs.data(), lhs.size(), rhs.data(), rhs.size());
            }

            bool operator()(const std::vector<D3DVERTEXELEMENT9>& lhs, std::span<const D3DVERTEXELEMENT9> rhs) const noexcept
            {
                return compare(lhs.data(), lhs.size(), rhs.data(), rhs.size());
            }

            bool operator()(std::span<const D3DVERTEXELEMENT9> lhs, const std::vector<D3DVERTEXELEMENT9>& rhs) const noexcept
            {
                return compare(lhs.data(), lhs.size(), rhs.data(), rhs.size());
            }

        private:
            bool compare(const D3DVERTEXELEMENT9* p1, size_t n1, const D3DVERTEXELEMENT9* p2, size_t n2) const noexcept
            {
                if (n1 != n2)
                    return false;
                
                if (n1 == 0)
                    return true;

                return std::memcmp(p1, p2, n1 * sizeof(D3DVERTEXELEMENT9)) == 0;
            }
        };

        std::unordered_map<std::vector<D3DVERTEXELEMENT9>, LPDIRECT3DVERTEXDECLARATION9, ElementsHasher, ElementsEqual> m_cache;
    };
}

