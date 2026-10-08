#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#endif

#include "ShaderRegistryCache.h"

namespace EterLib::Render
{
    uint16_t ShaderRegistryCache::GetOrCreateShaderPairId(LPDIRECT3DVERTEXSHADER9 vs, LPDIRECT3DPIXELSHADER9 ps, DWORD fvf) noexcept
    {
        ShaderBinding binding{ vs, ps, fvf };
        
        auto it = m_bindingToId.find(binding);
        if (it != m_bindingToId.end())
        {
            return it->second;
        }

        // We use up to MAX_ID, inclusive. Total slots = MAX_ID + 1 = 4096.
        if (m_idToBinding.size() > MAX_ID)
        {
            // If cache is full, we could just return a special ID, or overwrite, but for now 
            // returning 0 or the max ID might be dangerous. Let's return the last mapped ID 
            // if we exceed 12-bit limit, or maybe 0 with logging.
            // Requirement says 0..4095. If it's full, just return 0 to avoid crash, 
            // or perhaps don't cache and return 0.
            return 0; 
        }

        uint16_t newId = static_cast<uint16_t>(m_idToBinding.size());
        m_bindingToId[binding] = newId;
        m_idToBinding.push_back(binding);

        return newId;
    }

    ShaderBinding ShaderRegistryCache::GetBindingById(uint16_t id) const noexcept
    {
        if (id < m_idToBinding.size())
        {
            return m_idToBinding[id];
        }
        
        // Return default empty binding if ID is out of bounds
        return ShaderBinding{ nullptr, nullptr, 0 };
    }

    void ShaderRegistryCache::Clear() noexcept
    {
        m_bindingToId.clear();
        m_idToBinding.clear();
    }
}

