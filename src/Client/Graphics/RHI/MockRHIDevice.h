#pragma once

#include "RHICore.h"
#include <cstdint>

namespace Client::Graphics::RHI
{

struct MockViewport
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minZ = 0.0f;
    float maxZ = 1.0f;
};

class MockRHICommandList : public IRHICommandList
{
public:
    MockRHICommandList() = default;
    ~MockRHICommandList() override = default;

    void Clear(uint32_t flags, float r, float g, float b, float a, float depth, uint32_t stencil) override;
    void SetViewport(float x, float y, float width, float height, float minZ = 0.0f, float maxZ = 1.0f) override;
    void Draw(uint32_t vertexCount, uint32_t startVertex = 0) override;
    void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) override;

    [[nodiscard]] uint32_t GetDrawCallCount() const noexcept { return m_drawCallCount; }
    [[nodiscard]] uint32_t GetTotalVertexCount() const noexcept { return m_totalVertexCount; }
    [[nodiscard]] uint32_t GetTotalIndexCount() const noexcept { return m_totalIndexCount; }
    [[nodiscard]] uint32_t GetClearCount() const noexcept { return m_clearCount; }
    [[nodiscard]] const MockViewport& GetLastViewport() const noexcept { return m_lastViewport; }

    void ResetStats() noexcept;

private:
    uint32_t m_drawCallCount = 0;
    uint32_t m_totalVertexCount = 0;
    uint32_t m_totalIndexCount = 0;
    uint32_t m_clearCount = 0;
    MockViewport m_lastViewport{};
};

class MockRHIDevice : public IRHIDevice
{
public:
    struct Stats
    {
        uint32_t frameCount = 0;
        uint32_t errors = 0;
    };

    MockRHIDevice() = default;
    ~MockRHIDevice() override = default;

    bool Initialize(void* windowHandle, uint32_t width, uint32_t height) override;
    bool BeginFrame() override;
    void EndFrame() override;
    bool Present() override;
    void Resize(uint32_t width, uint32_t height) override;
    IRHICommandList* GetCommandList() override;

    [[nodiscard]] bool IsInitialized() const noexcept { return m_isInitialized; }
    [[nodiscard]] bool IsInFrame() const noexcept { return m_isInFrame; }
    [[nodiscard]] uint32_t GetFrameCount() const noexcept { return m_stats.frameCount; }
    [[nodiscard]] uint32_t GetErrorCount() const noexcept { return m_stats.errors; }
    [[nodiscard]] const Stats& GetStats() const noexcept { return m_stats; }
    [[nodiscard]] MockRHICommandList* GetMockCommandList() noexcept { return &m_commandList; }

    void SimulateError() noexcept { m_stats.errors++; }

private:
    bool m_isInitialized = false;
    bool m_isInFrame = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    Stats m_stats{};
    MockRHICommandList m_commandList;
};

} // namespace Client::Graphics::RHI
