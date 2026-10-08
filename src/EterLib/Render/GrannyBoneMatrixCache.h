#pragma once

#ifndef MOCK_TESTING
#include "../StdAfx.h"
#else
#ifndef D3DMATRIX_DEFINED
#define D3DMATRIX_DEFINED
struct D3DMATRIX {
    float m[4][4];
};
#endif
#endif

#include <span>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <list>

namespace EterLib::Render {

    class GrannyBoneMatrixCache {
    public:
        explicit GrannyBoneMatrixCache(size_t maxBonesCapacity = 65536);
        ~GrannyBoneMatrixCache() = default;

        GrannyBoneMatrixCache(const GrannyBoneMatrixCache&) = delete;
        GrannyBoneMatrixCache& operator=(const GrannyBoneMatrixCache&) = delete;

        bool TryGetBones(uint64_t animKey, std::span<D3DMATRIX>& outBones);
        void StoreBones(uint64_t animKey, std::span<const D3DMATRIX> bones);
        void Clear() noexcept;

    private:
        struct CacheEntry {
            uint64_t animKey;
            std::vector<D3DMATRIX> bones;
        };

        size_t m_maxBonesCapacity;
        size_t m_currentBonesCount;

        std::list<CacheEntry> m_lruList;
        std::unordered_map<uint64_t, std::list<CacheEntry>::iterator> m_cacheMap;

        std::vector<std::vector<D3DMATRIX>> m_pool;

        std::vector<D3DMATRIX> GetFromPool(size_t requiredSize);
        void ReturnToPool(std::vector<D3DMATRIX>&& vec);
        void EvictIfNeeded(size_t requiredSize);
    };

}

