#define _D3D9_H_
#define _D3DX9_H_

struct D3DVIEWPORT9 {
    unsigned long X;
    unsigned long Y;
    unsigned long Width;
    unsigned long Height;
    float MinZ;
    float MaxZ;
};

struct IDirect3DDevice9 {
    virtual ~IDirect3DDevice9() = default;
    virtual long BeginScene() = 0;
    virtual long EndScene() = 0;
    virtual long SetViewport(D3DVIEWPORT9* pViewport) = 0;
    virtual long Clear(unsigned long Count, const void* pRects, unsigned long Flags, unsigned long Color, float Z, unsigned long Stencil) = 0;
};

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "EterLib/Render/IRenderHardwareInterface.h"
#include "EterLib/Render/D3D9RenderHardwareInterface.h"

using namespace testing;
using namespace EterLib::Render;

class MockIDirect3DDevice9 : public IDirect3DDevice9 {
public:
    MOCK_METHOD(long, BeginScene, (), (override));
    MOCK_METHOD(long, EndScene, (), (override));
    MOCK_METHOD(long, SetViewport, (D3DVIEWPORT9*), (override));
    MOCK_METHOD(long, Clear, (unsigned long, const void*, unsigned long, unsigned long, float, unsigned long), (override));
};

class MockRenderHardwareInterface : public IRenderHardwareInterface {
public:
    MOCK_METHOD(void, BeginFrame, (), (override));
    MOCK_METHOD(void, EndFrame, (), (override));
    MOCK_METHOD(void, SetViewport, (int, int, int, int), (override));
    MOCK_METHOD(void, Clear, (uint32_t, uint32_t, float), (override));
};

TEST(RenderHardwareInterfaceTest, MockUsage) {
    MockRenderHardwareInterface mockRhi;

    EXPECT_CALL(mockRhi, BeginFrame()).Times(1);
    EXPECT_CALL(mockRhi, Clear(1, 0xFFFFFFFF, 1.0f)).Times(1);
    EXPECT_CALL(mockRhi, SetViewport(0, 0, 800, 600)).Times(1);
    EXPECT_CALL(mockRhi, EndFrame()).Times(1);

    mockRhi.BeginFrame();
    mockRhi.Clear(1, 0xFFFFFFFF, 1.0f);
    mockRhi.SetViewport(0, 0, 800, 600);
    mockRhi.EndFrame();
}

TEST(RenderHardwareInterfaceTest, D3D9Implementation) {
    MockIDirect3DDevice9 mockDevice;
    D3D9RenderHardwareInterface d3d9Rhi(&mockDevice);

    EXPECT_CALL(mockDevice, BeginScene()).Times(1);
    d3d9Rhi.BeginFrame();

    EXPECT_CALL(mockDevice, Clear(0, nullptr, 3, 0xFF000000, 0.5f, 0)).Times(1);
    d3d9Rhi.Clear(3, 0xFF000000, 0.5f);

    EXPECT_CALL(mockDevice, SetViewport(NotNull())).WillOnce([](D3DVIEWPORT9* vp) {
        EXPECT_EQ(vp->X, 10);
        EXPECT_EQ(vp->Y, 20);
        EXPECT_EQ(vp->Width, 1024);
        EXPECT_EQ(vp->Height, 768);
        EXPECT_FLOAT_EQ(vp->MinZ, 0.0f);
        EXPECT_FLOAT_EQ(vp->MaxZ, 1.0f);
        return 0;
    });
    d3d9Rhi.SetViewport(10, 20, 1024, 768);

    EXPECT_CALL(mockDevice, EndScene()).Times(1);
    d3d9Rhi.EndFrame();
}

