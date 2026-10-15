#include "MockRHIDevice.h"

namespace Client::Graphics::RHI
{

void MockRHICommandList::Clear(uint32_t flags, float r, float g, float b, float a, float depth, uint32_t stencil)
{
    (void)flags; (void)r; (void)g; (void)b; (void)a; (void)depth; (void)stencil;
    m_clearCount++;
}

void MockRHICommandList::SetViewport(float x, float y, float width, float height, float minZ, float maxZ)
{
    m_lastViewport = { x, y, width, height, minZ, maxZ };
}

void MockRHICommandList::Draw(uint32_t vertexCount, uint32_t startVertex)
{
    (void)startVertex;
    m_drawCallCount++;
    m_totalVertexCount += vertexCount;
}

void MockRHICommandList::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex)
{
    (void)startIndex; (void)baseVertex;
    m_drawCallCount++;
    m_totalIndexCount += indexCount;
}

void MockRHICommandList::ResetStats() noexcept
{
    m_drawCallCount = 0;
    m_totalVertexCount = 0;
    m_totalIndexCount = 0;
    m_clearCount = 0;
    m_lastViewport = {};
}

bool MockRHIDevice::Initialize(void* windowHandle, uint32_t width, uint32_t height)
{
    (void)windowHandle;
    m_width = width;
    m_height = height;
    m_isInitialized = true;
    m_isInFrame = false;
    m_commandList.SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    return true;
}

bool MockRHIDevice::BeginFrame()
{
    if (!m_isInitialized)
    {
        m_stats.errors++;
        return false;
    }
    if (m_isInFrame)
    {
        m_stats.errors++;
        return false;
    }
    m_isInFrame = true;
    return true;
}

void MockRHIDevice::EndFrame()
{
    if (!m_isInFrame)
    {
        m_stats.errors++;
        return;
    }
    m_isInFrame = false;
    m_stats.frameCount++;
}

bool MockRHIDevice::Present()
{
    if (m_isInFrame)
    {
        m_stats.errors++;
        return false;
    }
    return true;
}

void MockRHIDevice::Resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    m_commandList.SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
}

IRHICommandList* MockRHIDevice::GetCommandList()
{
    return &m_commandList;
}

} // namespace Client::Graphics::RHI
