#include "GrannyBoneMatrixCache.h"

namespace EterLib::Render {

    GrannyBoneMatrixCache::GrannyBoneMatrixCache(size_t maxBonesCapacity)
        : m_maxBonesCapacity(maxBonesCapacity), m_currentBonesCount(0)
    {
    }

    bool GrannyBoneMatrixCache::TryGetBones(uint64_t animKey, std::span<D3DMATRIX>& outBones)
    {
        auto it = m_cacheMap.find(animKey);
        if (it == m_cacheMap.end()) {
            return false;
        }

        m_lruList.splice(m_lruList.begin(), m_lruList, it->second);
        outBones = std::span<D3DMATRIX>(it->second->bones.data(), it->second->bones.size());
        return true;
    }

    void GrannyBoneMatrixCache::StoreBones(uint64_t animKey, std::span<const D3DMATRIX> bones)
    {
        if (bones.empty()) {
            return;
        }

        auto it = m_cacheMap.find(animKey);
        if (it != m_cacheMap.end()) {
            m_lruList.splice(m_lruList.begin(), m_lruList, it->second);
            if (it->second->bones.size() == bones.size()) {
                std::copy(bones.begin(), bones.end(), it->second->bones.begin());
            } else {
                m_currentBonesCount -= it->second->bones.size();
                ReturnToPool(std::move(it->second->bones));

                EvictIfNeeded(bones.size());
                it->second->bones = GetFromPool(bones.size());
                std::copy(bones.begin(), bones.end(), it->second->bones.begin());
                m_currentBonesCount += bones.size();
            }
            return;
        }

        EvictIfNeeded(bones.size());

        auto newBones = GetFromPool(bones.size());
        std::copy(bones.begin(), bones.end(), newBones.begin());

        m_lruList.push_front({ animKey, std::move(newBones) });
        m_cacheMap[animKey] = m_lruList.begin();
        m_currentBonesCount += bones.size();
    }

    void GrannyBoneMatrixCache::Clear() noexcept
    {
        for (auto& entry : m_lruList) {
            ReturnToPool(std::move(entry.bones));
        }
        m_lruList.clear();
        m_cacheMap.clear();
        m_currentBonesCount = 0;
    }

    std::vector<D3DMATRIX> GrannyBoneMatrixCache::GetFromPool(size_t requiredSize)
    {
        if (!m_pool.empty()) {
            auto vec = std::move(m_pool.back());
            m_pool.pop_back();
            vec.resize(requiredSize);
            return vec;
        }
        return std::vector<D3DMATRIX>(requiredSize);
    }

    void GrannyBoneMatrixCache::ReturnToPool(std::vector<D3DMATRIX>&& vec)
    {
        vec.clear();
        m_pool.push_back(std::move(vec));
    }

    void GrannyBoneMatrixCache::EvictIfNeeded(size_t requiredSize)
    {
        if (requiredSize > m_maxBonesCapacity) {
            // Cannot satisfy anyway, but let's clear as much as possible up to max capacity logic
            // In reality, we might want to just not cache if it's too big, but we follow standard LRU.
        }

        while (!m_lruList.empty() && m_currentBonesCount + requiredSize > m_maxBonesCapacity) {
            auto& last = m_lruList.back();
            m_currentBonesCount -= last.bones.size();
            m_cacheMap.erase(last.animKey);
            ReturnToPool(std::move(last.bones));
            m_lruList.pop_back();
        }
    }

}

