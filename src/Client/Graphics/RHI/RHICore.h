#pragma once

#include <cstdint>

namespace Client::Graphics::RHI
{

class IRHICommandList
{
public:
    virtual ~IRHICommandList() = default;

    virtual void Clear(uint32_t flags, float r, float g, float b, float a, float depth, uint32_t stencil) = 0;
    virtual void SetViewport(float x, float y, float width, float height, float minZ = 0.0f, float maxZ = 1.0f) = 0;
    virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) = 0;
};

class IRHIDevice
{
public:
    virtual ~IRHIDevice() = default;

    virtual bool Initialize(void* windowHandle, uint32_t width, uint32_t height) = 0;
    virtual bool BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual bool Present() = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;
    virtual IRHICommandList* GetCommandList() = 0;
};

} // namespace Client::Graphics::RHI
