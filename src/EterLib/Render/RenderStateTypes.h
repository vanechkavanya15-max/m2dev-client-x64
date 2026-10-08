#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <cstdint>
#include <cstddef>
#include <array>
#include <span>
#include <utility>
#include <type_traits>

namespace EterLib::Render {

// ============================================================================
// Silnie typowane enumy domenowe dla maszyny stanow D3D9 (Standard C++23)
// ============================================================================

enum class BlendMode : uint8_t {
    Opaque,
    AlphaBlend,         // D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA
    Additive,           // D3DBLEND_SRCALPHA, D3DBLEND_ONE
    Subtractive,
    Multiply,           // D3DBLEND_DESTCOLOR, D3DBLEND_ZERO
    AlphaAdd,           // D3DBLEND_ONE, D3DBLEND_ONE
    Custom
};

enum class DepthMode : uint8_t {
    ReadWrite,          // ZEnable = TRUE, ZWrite = TRUE, ZFunc = LESSEQUAL
    ReadOnly,           // ZEnable = TRUE, ZWrite = FALSE, ZFunc = LESSEQUAL
    Disabled,           // ZEnable = FALSE, ZWrite = FALSE
    EqualTest,          // ZEnable = TRUE, ZWrite = FALSE, ZFunc = EQUAL
    AlwaysWrite         // ZEnable = TRUE, ZWrite = TRUE, ZFunc = ALWAYS
};

enum class CullMode : uint8_t {
    None = D3DCULL_NONE,
    Clockwise = D3DCULL_CW,
    CounterClockwise = D3DCULL_CCW
};

enum class FillMode : uint8_t {
    Solid = D3DFILL_SOLID,
    Wireframe = D3DFILL_WIREFRAME,
    Point = D3DFILL_POINT
};

enum class FogMode : uint8_t {
    Disabled,
    Linear,
    Exp,
    Exp2
};

enum class SamplerFilterMode : uint8_t {
    Point,
    Linear,
    Bilinear,
    Trilinear,
    Anisotropic
};

enum class SamplerAddressMode : uint8_t {
    Wrap = D3DTADDRESS_WRAP,
    Clamp = D3DTADDRESS_CLAMP,
    Border = D3DTADDRESS_BORDER,
    Mirror = D3DTADDRESS_MIRROR
};

enum class TextureColorOp : uint8_t {
    Disable = D3DTOP_DISABLE,
    SelectArg1 = D3DTOP_SELECTARG1,
    SelectArg2 = D3DTOP_SELECTARG2,
    Modulate = D3DTOP_MODULATE,
    Modulate2X = D3DTOP_MODULATE2X,
    Modulate4X = D3DTOP_MODULATE4X,
    Add = D3DTOP_ADD,
    AddSigned = D3DTOP_ADDSIGNED,
    Subtract = D3DTOP_SUBTRACT
};

enum class AlphaTestFunc : uint8_t {
    Never = D3DCMP_NEVER,
    Less = D3DCMP_LESS,
    Equal = D3DCMP_EQUAL,
    LessEqual = D3DCMP_LESSEQUAL,
    Greater = D3DCMP_GREATER,
    NotEqual = D3DCMP_NOTEQUAL,
    GreaterEqual = D3DCMP_GREATEREQUAL,
    Always = D3DCMP_ALWAYS
};

// ============================================================================
// Stale konfiguracyjne potoku D3D9
// ============================================================================
constexpr size_t MAX_D3D_RENDERSTATES = 256;
constexpr size_t MAX_D3D_TEXTURE_STAGES = 8;
constexpr size_t MAX_D3D_TEXTURE_STATES = 128;
constexpr size_t MAX_D3D_SAMPLERS = 8;
constexpr size_t MAX_D3D_SAMPLER_STATES = 14;
constexpr size_t MAX_D3D_STREAMS = 16;
constexpr size_t MAX_D3D_LIGHTS = 8;
constexpr size_t DEFAULT_STATE_STACK_DEPTH = 8;

// ============================================================================
// Wpis przywracania stanu dla straznikow RAII (Zero heap allocation)
// ============================================================================
template <typename TKey, typename TValue>
struct StateRestoreEntry {
    TKey key{};
    TValue oldValue{};
};

// ============================================================================
// Baza konceptualna dla wszystkich straznikow RAII w EterLib::Render
// ============================================================================
template <typename T>
concept StateScopeConcept = requires(T scope) {
    { scope.Restore() } -> std::same_as<void>;
};

} // namespace EterLib::Render
