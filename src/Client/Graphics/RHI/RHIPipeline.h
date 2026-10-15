#pragma once

#include <cstdint>

namespace Client::Graphics::RHI
{

enum class BlendFactor : uint8_t
{
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DestAlpha,
    InvDestAlpha,
    DestColor,
    InvDestColor
};

enum class BlendOp : uint8_t
{
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max
};

struct RHIBlendState
{
    bool blendEnable = false;
    BlendFactor srcBlend = BlendFactor::One;
    BlendFactor destBlend = BlendFactor::Zero;
    BlendOp blendOp = BlendOp::Add;
};

enum class CompareFunc : uint8_t
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

struct RHIDepthStencilState
{
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    CompareFunc depthFunc = CompareFunc::LessEqual;
};

enum class CullMode : uint8_t
{
    None,
    CW,
    CCW
};

enum class FillMode : uint8_t
{
    Solid,
    Wireframe
};

struct RHIRasterizerState
{
    CullMode cullMode = CullMode::CCW;
    FillMode fillMode = FillMode::Solid;
};

class IRHIPipelineState
{
public:
    virtual ~IRHIPipelineState() = default;

    [[nodiscard]] virtual const RHIBlendState& GetBlendState() const noexcept = 0;
    [[nodiscard]] virtual const RHIDepthStencilState& GetDepthStencilState() const noexcept = 0;
    [[nodiscard]] virtual const RHIRasterizerState& GetRasterizerState() const noexcept = 0;
};

} // namespace Client::Graphics::RHI
