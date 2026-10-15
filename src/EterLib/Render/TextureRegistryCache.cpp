#include "TextureRegistryCache.h"

namespace EterLib::Render {

TextureRegistryCache::TextureRegistryCache() noexcept {
    // Initialize id 0 as the null texture
    m_idToTexture.push_back(nullptr);
}

uint32_t TextureRegistryCache::GetOrCreateTextureId(LPDIRECT3DTEXTURE9 texture) noexcept {
    if (!texture) {
        return NULL_TEXTURE_ID;
    }

    if (auto it = m_textureToId.find(texture); it != m_textureToId.end()) {
        return it->second;
    }

    uint32_t newId = 0;
    if (!m_freeIds.empty()) {
        newId = m_freeIds.back();
        m_freeIds.pop_back();
        m_idToTexture[newId] = texture;
    } else {
        newId = static_cast<uint32_t>(m_idToTexture.size());
        if (newId > MAX_TEXTURE_ID) {
            // Reached max ID limit, return NULL_TEXTURE_ID to indicate failure
            // though depending on the context, we could assert or log here.
            return NULL_TEXTURE_ID;
        }
        m_idToTexture.push_back(texture);
    }

    m_textureToId[texture] = newId;
    return newId;
}

LPDIRECT3DTEXTURE9 TextureRegistryCache::GetTextureById(uint32_t id) const noexcept {
    if (id == NULL_TEXTURE_ID || id >= m_idToTexture.size()) {
        return nullptr;
    }
    return m_idToTexture[id];
}

void TextureRegistryCache::Invalidate(LPDIRECT3DTEXTURE9 texture) noexcept {
    if (!texture) {
        return;
    }

    auto it = m_textureToId.find(texture);
    if (it != m_textureToId.end()) {
        uint32_t id = it->second;
        m_idToTexture[id] = nullptr;
        m_freeIds.push_back(id);
        m_textureToId.erase(it);
    }
}

void TextureRegistryCache::Clear() noexcept {
    m_textureToId.clear();
    m_idToTexture.clear();
    m_freeIds.clear();

    // Re-initialize id 0 as the null texture
    m_idToTexture.push_back(nullptr);
}

} // namespace EterLib::Render

