#pragma once

#include "RHICore.h"
#include "Client/Graphics/ZeroOverheadStateCache.h"

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d9.h>
#endif

struct IDirect3DDevice9;

namespace Client::Graphics::RHI
{

class D3D9RHICommandList : public IRHICommandList
{
public:
    explicit D3D9RHICommandList(IDirect3DDevice9* device = nullptr) noexcept : m_device(device) {}
    ~D3D9RHICommandList() override = default;

    void SetDevice(IDirect3DDevice9* device) noexcept { m_device = device; }

    void Clear(uint32_t flags, float r, float g, float b, float a, float depth, uint32_t stencil) override;
    void SetViewport(float x, float y, float width, float height, float minZ = 0.0f, float maxZ = 1.0f) override;
    void Draw(uint32_t vertexCount, uint32_t startVertex = 0) override;
    void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) override;

private:
    IDirect3DDevice9* m_device = nullptr;
};

class D3D9RHIDevice : public IRHIDevice
{
public:
    D3D9RHIDevice() noexcept;
    ~D3D9RHIDevice() noexcept override;

    D3D9RHIDevice(const D3D9RHIDevice&) = delete;
    D3D9RHIDevice& operator=(const D3D9RHIDevice&) = delete;
    D3D9RHIDevice(D3D9RHIDevice&&) = delete;
    D3D9RHIDevice& operator=(D3D9RHIDevice&&) = delete;

    bool Initialize(void* windowHandle, uint32_t width, uint32_t height) override;
    bool InitializeWithDevice(IDirect3DDevice9* device) noexcept;

    bool BeginFrame() override;
    void EndFrame() override;
    bool Present() override;
    void Resize(uint32_t width, uint32_t height) override;
    IRHICommandList* GetCommandList() override;

    [[nodiscard]] ZeroOverheadStateCache& GetStateCache() noexcept { return m_stateCache; }
    [[nodiscard]] const ZeroOverheadStateCache& GetStateCache() const noexcept { return m_stateCache; }
    [[nodiscard]] IDirect3DDevice9* GetDevice() const noexcept { return m_device; }
    [[nodiscard]] bool IsInFrame() const noexcept { return m_isFrameActive; }

private:
    IDirect3DDevice9* m_device = nullptr;
    ZeroOverheadStateCache m_stateCache;
    D3D9RHICommandList m_commandList;
    bool m_isFrameActive = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Client::Graphics::RHI
