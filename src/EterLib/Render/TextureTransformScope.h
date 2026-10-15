#pragma once

#include <cstdint>

namespace EterLib::Render
{
    /**
     * @brief A RAII scope guard for managing texture transformation and coordinate indices.
     * 
     * Useful for UV animations (e.g., scrolling water or effects).
     * Ensures that the texture transform flags and texture coordinate index are safely 
     * disabled/restored to defaults upon exiting the scope.
     */
    class TextureTransformScope
    {
    public:
        /**
         * @brief Applies the specified texture transform flags and coordinate index.
         * 
         * @param stage The texture stage to apply the transform to.
         * @param transformFlags The transform flags (e.g., D3DTTFF_COUNT2, etc.).
         * @param texCoordIndex The texture coordinate index to use.
         */
        TextureTransformScope(uint32_t stage, uint32_t transformFlags, uint32_t texCoordIndex)
            : m_stage(stage)
        {
            STATEMANAGER.SetTextureStageState(m_stage, D3DTSS_TEXTURETRANSFORMFLAGS, transformFlags);
            STATEMANAGER.SetTextureStageState(m_stage, D3DTSS_TEXCOORDINDEX, texCoordIndex);
        }

        ~TextureTransformScope()
        {
            // Restore to default (Disable)
            // 0 is D3DTTFF_DISABLE for texture transform flags.
            // m_stage is the default index mapping for the texture coordinate.
            STATEMANAGER.SetTextureStageState(m_stage, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
            STATEMANAGER.SetTextureStageState(m_stage, D3DTSS_TEXCOORDINDEX, m_stage);
        }

        // Prevent copying and moving
        TextureTransformScope(const TextureTransformScope&) = delete;
        TextureTransformScope& operator=(const TextureTransformScope&) = delete;
        TextureTransformScope(TextureTransformScope&&) = delete;
        TextureTransformScope& operator=(TextureTransformScope&&) = delete;

    private:
        uint32_t m_stage;
    };
}
