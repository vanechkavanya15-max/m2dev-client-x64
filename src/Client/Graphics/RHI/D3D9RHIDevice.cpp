#include "D3D9RHIDevice.h"
#include "EterBase/LogModern.h"

namespace Client::Graphics::RHI
{

void D3D9RHICommandList::Clear(uint32_t flags, float r, float g, float b, float a, float depth, uint32_t stencil)
{
#if defined(_WIN32) || defined(_WIN64)
    if (!m_device)
        return;

    uint8_t byteR = static_cast<uint8_t>(r * 255.0f);
    uint8_t byteG = static_cast<uint8_t>(g * 255.0f);
    uint8_t byteB = static_cast<uint8_t>(b * 255.0f);
    uint8_t byteA = static_cast<uint8_t>(a * 255.0f);
    D3DCOLOR color = D3DCOLOR_ARGB(byteA, byteR, byteG, byteB);

    m_device->Clear(0, nullptr, flags, color, depth, stencil);
#else
    (void)flags; (void)r; (void)g; (void)b; (void)a; (void)depth; (void)stencil;
#endif
}

void D3D9RHICommandList::SetViewport(float x, float y, float width, float height, float minZ, float maxZ)
{
#if defined(_WIN32) || defined(_WIN64)
    if (!m_device)
        return;

    D3DVIEWPORT9 vp{};
    vp.X = static_cast<DWORD>(x);
    vp.Y = static_cast<DWORD>(y);
    vp.Width = static_cast<DWORD>(width);
    vp.Height = static_cast<DWORD>(height);
    vp.MinZ = minZ;
    vp.MaxZ = maxZ;

    m_device->SetViewport(&vp);
#else
    (void)x; (void)y; (void)width; (void)height; (void)minZ; (void)maxZ;
#endif
}

void D3D9RHICommandList::Draw(uint32_t vertexCount, uint32_t startVertex)
{
#if defined(_WIN32) || defined(_WIN64)
    if (!m_device || vertexCount < 3)
        return;

    uint32_t primitiveCount = vertexCount / 3;
    m_device->DrawPrimitive(D3DPT_TRIANGLELIST, startVertex, primitiveCount);
#else
    (void)vertexCount; (void)startVertex;
#endif
}

void D3D9RHICommandList::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex)
{
#if defined(_WIN32) || defined(_WIN64)
    if (!m_device || indexCount < 3)
        return;

    uint32_t primitiveCount = indexCount / 3;
    m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, baseVertex, 0, indexCount, startIndex, primitiveCount);
#else
    (void)indexCount; (void)startIndex; (void)baseVertex;
#endif
}

D3D9RHIDevice::D3D9RHIDevice() noexcept
{
}

D3D9RHIDevice::~D3D9RHIDevice() noexcept
{
    m_device = nullptr;
}

bool D3D9RHIDevice::Initialize(void* windowHandle, uint32_t width, uint32_t height)
{
    (void)windowHandle;
    m_width = width;
    m_height = height;
    return true;
}

bool D3D9RHIDevice::InitializeWithDevice(IDirect3DDevice9* device) noexcept
{
    if (!device)
        return false;

    m_device = device;
    m_stateCache.BindDevice(device);
    m_stateCache.ResetToDefaults();
    m_commandList.SetDevice(device);
    return true;
}

bool D3D9RHIDevice::BeginFrame()
{
#if defined(_WIN32) || defined(_WIN64)
    if (!m_device)
        return false;

    if (m_isFrameActive)
        return true;

    HRESULT hr = m_device->BeginScene();
    if (SUCCEEDED(hr))
    {
        m_isFrameActive = true;
        return true;
    }
    return false;
#else
    return false;
#endif
}

void D3D9RHIDevice::EndFrame()
{
#if defined(_WIN32) || defined(_WIN64)
    if (!m_device || !m_isFrameActive)
        return;

    m_device->EndScene();
    m_isFrameActive = false;
#endif
}

bool D3D9RHIDevice::Present()
{
#if defined(_WIN32) || defined(_WIN64)
    if (!m_device)
        return false;

    HRESULT hr = m_device->Present(nullptr, nullptr, nullptr, nullptr);
    return SUCCEEDED(hr);
#else
    return false;
#endif
}

void D3D9RHIDevice::Resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    m_commandList.SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
}

IRHICommandList* D3D9RHIDevice::GetCommandList()
{
    return &m_commandList;
}

} // namespace Client::Graphics::RHI
