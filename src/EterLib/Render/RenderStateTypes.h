#pragma once

namespace EterLib::Render {

/**
 * @brief Enum representing common blending presets.
 */
enum class BlendMode {
    Opaque,     ///< No blending.
    AlphaBlend, ///< Standard alpha blending.
    Additive,   ///< Additive blending.
    Multiply    ///< Multiply blending.
};

} // namespace EterLib::Render
