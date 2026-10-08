#include "MaterialRegistryCache.h"
#include "../../EterBase/LogModern.h"

namespace EterLib::Render
{
    uint8_t MaterialRegistryCache::GetOrCreateMaterialId(const D3DMATERIAL9& mat) noexcept
    {
        auto it = m_materialToId.find(mat);
        if (it != m_materialToId.end())
        {
            return it->second;
        }

        if (m_idToMaterial.size() >= 256)
        {
            EterBase::ModernLogger::Error("MaterialRegistryCache limit exceeded (256 materials max). Returning ID 0.");
            return 0; // Osiagnieto limit 8 bitow, zwroc domyslny (jezeli istnieje) lub 0
        }

        uint8_t newId = static_cast<uint8_t>(m_idToMaterial.size());
        m_idToMaterial.push_back(mat);
        m_materialToId[mat] = newId;

        return newId;
    }

    const D3DMATERIAL9& MaterialRegistryCache::GetMaterialById(uint8_t id) const noexcept
    {
        if (id < m_idToMaterial.size())
        {
            return m_idToMaterial[id];
        }

        // Jezeli ID jest spoza zakresu, mozna zwrocic pierwszy dostepny albo rzucic bladem
        // poniewaz jest noexcept zwracamy index 0 albo pusty.
        if (!m_idToMaterial.empty())
        {
            EterBase::ModernLogger::Error("MaterialRegistryCache::GetMaterialById ID {} out of bounds, returning index 0.", id);
            return m_idToMaterial[0];
        }

        // W absolutnym przypadku braku danych, zdefiniujmy statyczny default
        static D3DMATERIAL9 s_defaultMaterial{};
        return s_defaultMaterial;
    }

    void MaterialRegistryCache::Clear() noexcept
    {
        m_materialToId.clear();
        m_idToMaterial.clear();
    }
} // namespace EterLib::Render

