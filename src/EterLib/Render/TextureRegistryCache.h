#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

struct IDirect3DTexture9;
using LPDIRECT3DTEXTURE9 = IDirect3DTexture9*;

namespace EterLib::Render {

class TextureRegistryCache {
public:
    static constexpr uint32_t MAX_TEXTURE_ID = 1048575; // 20-bit limit
    static constexpr uint32_t NULL_TEXTURE_ID = 0;

    TextureRegistryCache() noexcept;
    ~TextureRegistryCache() noexcept = default;

    // Non-copyable, non-movable
    TextureRegistryCache(const TextureRegistryCache&) = delete;
    TextureRegistryCache& operator=(const TextureRegistryCache&) = delete;
    TextureRegistryCache(TextureRegistryCache&&) = delete;
    TextureRegistryCache& operator=(TextureRegistryCache&&) = delete;

    uint32_t GetOrCreateTextureId(LPDIRECT3DTEXTURE9 texture) noexcept;
    LPDIRECT3DTEXTURE9 GetTextureById(uint32_t id) const noexcept;
    void Invalidate(LPDIRECT3DTEXTURE9 texture) noexcept;
    void Clear() noexcept;

private:
    std::unordered_map<LPDIRECT3DTEXTURE9, uint32_t> m_textureToId;
    std::vector<LPDIRECT3DTEXTURE9> m_idToTexture;
    std::vector<uint32_t> m_freeIds;
};

} // namespace EterLib::Render

