#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <vector>

// Prevent real d3d9.h from loading on MSVC to avoid COM massive pure virtual boilerplate
#ifndef _D3D9_H_
#define _D3D9_H_
#endif
#ifndef __D3D9_H__
#define __D3D9_H__
#endif

// Mock definitions for Windows/DirectX types
typedef unsigned int UINT;
typedef int INT;
typedef long HRESULT;

#define S_OK 0
#define E_FAIL -1

enum D3DPRIMITIVETYPE
{
    D3DPT_POINTLIST = 1,
    D3DPT_LINELIST = 2,
    D3DPT_LINESTRIP = 3,
    D3DPT_TRIANGLELIST = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN = 6,
    D3DPT_FORCE_DWORD = 0x7fffffff
};

//
// Mock implementation of IDirect3DVertexBuffer9
//
struct IDirect3DVertexBuffer9
{
    int dummy;
};
typedef IDirect3DVertexBuffer9* LPDIRECT3DVERTEXBUFFER9;

//
// Mock implementation of IDirect3DIndexBuffer9
//
struct IDirect3DIndexBuffer9
{
    int dummy;
};
typedef IDirect3DIndexBuffer9* LPDIRECT3DINDEXBUFFER9;

//
// Mock device implementation to capture Direct3D method calls
//
struct IDirect3DDevice9
{
    struct SetStreamSourceCall
    {
        UINT StreamNumber;
        LPDIRECT3DVERTEXBUFFER9 pStreamData;
        UINT OffsetInBytes;
        UINT Stride;
    };

    struct SetIndicesCall
    {
        LPDIRECT3DINDEXBUFFER9 pIndexData;
    };

    struct DrawIndexedPrimitiveCall
    {
        D3DPRIMITIVETYPE Type;
        INT BaseVertexIndex;
        UINT MinVertexIndex;
        UINT NumVertices;
        UINT startIndex;
        UINT primCount;
    };

    std::vector<SetStreamSourceCall> setStreamSourceCalls;
    std::vector<SetIndicesCall> setIndicesCalls;
    std::vector<DrawIndexedPrimitiveCall> drawIndexedPrimitiveCalls;

    HRESULT SetStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT OffsetInBytes, UINT Stride)
    {
        setStreamSourceCalls.push_back({StreamNumber, pStreamData, OffsetInBytes, Stride});
        return S_OK;
    }

    HRESULT SetIndices(LPDIRECT3DINDEXBUFFER9 pIndexData)
    {
        setIndicesCalls.push_back({pIndexData});
        return S_OK;
    }

    HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
    {
        drawIndexedPrimitiveCalls.push_back({Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount});
        return S_OK;
    }
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;

// Include the command to test
#include "../src/EterLib/Render/DrawIndexedCommand.h"

using namespace EterLib::Render;

TEST_SUITE("DrawIndexedCommand_Test_Suite")
{
    TEST_CASE("DrawIndexedCommand_Execute_successfully_calls_expected_D3D9_methods")
    {
        IDirect3DDevice9 mockDevice;
        IDirect3DVertexBuffer9 mockVB;
        IDirect3DIndexBuffer9 mockIB;

        DrawIndexedCommand cmd{};
        cmd.primitiveType = D3DPT_TRIANGLELIST;
        cmd.baseVertexIndex = 10;
        cmd.minVertexIndex = 0;
        cmd.numVertices = 100;
        cmd.startIndex = 0;
        cmd.primitiveCount = 50;
        cmd.vertexBuffer = &mockVB;
        cmd.indexBuffer = &mockIB;
        cmd.stride = 32;

        cmd.Execute(&mockDevice);

        REQUIRE(mockDevice.setStreamSourceCalls.size() == 1);
        CHECK(mockDevice.setStreamSourceCalls[0].StreamNumber == 0);
        CHECK(mockDevice.setStreamSourceCalls[0].pStreamData == &mockVB);
        CHECK(mockDevice.setStreamSourceCalls[0].OffsetInBytes == 0);
        CHECK(mockDevice.setStreamSourceCalls[0].Stride == 32);

        REQUIRE(mockDevice.setIndicesCalls.size() == 1);
        CHECK(mockDevice.setIndicesCalls[0].pIndexData == &mockIB);

        REQUIRE(mockDevice.drawIndexedPrimitiveCalls.size() == 1);
        CHECK(mockDevice.drawIndexedPrimitiveCalls[0].Type == D3DPT_TRIANGLELIST);
        CHECK(mockDevice.drawIndexedPrimitiveCalls[0].BaseVertexIndex == 10);
        CHECK(mockDevice.drawIndexedPrimitiveCalls[0].MinVertexIndex == 0);
        CHECK(mockDevice.drawIndexedPrimitiveCalls[0].NumVertices == 100);
        CHECK(mockDevice.drawIndexedPrimitiveCalls[0].startIndex == 0);
        CHECK(mockDevice.drawIndexedPrimitiveCalls[0].primCount == 50);
    }

    TEST_CASE("DrawIndexedCommand_Execute_skips_null_buffers_and_only_draws")
    {
        IDirect3DDevice9 mockDevice;

        DrawIndexedCommand cmd{};
        cmd.primitiveType = D3DPT_POINTLIST;
        cmd.baseVertexIndex = 0;
        cmd.minVertexIndex = 0;
        cmd.numVertices = 10;
        cmd.startIndex = 0;
        cmd.primitiveCount = 5;
        cmd.vertexBuffer = nullptr;
        cmd.indexBuffer = nullptr;
        cmd.stride = 16;

        cmd.Execute(&mockDevice);

        CHECK(mockDevice.setStreamSourceCalls.empty());
        CHECK(mockDevice.setIndicesCalls.empty());

        REQUIRE(mockDevice.drawIndexedPrimitiveCalls.size() == 1);
        CHECK(mockDevice.drawIndexedPrimitiveCalls[0].Type == D3DPT_POINTLIST);
    }

    TEST_CASE("DrawIndexedCommand_Execute_does_nothing_when_device_is_null")
    {
        DrawIndexedCommand cmd{};
        cmd.primitiveType = D3DPT_TRIANGLESTRIP;
        cmd.Execute(nullptr);
        // Execution simply returns cleanly without crashing.
        CHECK(true);
    }
    
    TEST_CASE("DrawIndexedCommand_Execute_with_only_vertex_buffer")
    {
        IDirect3DDevice9 mockDevice;
        IDirect3DVertexBuffer9 mockVB;

        DrawIndexedCommand cmd{};
        cmd.primitiveType = D3DPT_LINELIST;
        cmd.baseVertexIndex = 0;
        cmd.minVertexIndex = 0;
        cmd.numVertices = 10;
        cmd.startIndex = 0;
        cmd.primitiveCount = 5;
        cmd.vertexBuffer = &mockVB;
        cmd.indexBuffer = nullptr;
        cmd.stride = 24;

        cmd.Execute(&mockDevice);

        REQUIRE(mockDevice.setStreamSourceCalls.size() == 1);
        CHECK(mockDevice.setStreamSourceCalls[0].StreamNumber == 0);
        CHECK(mockDevice.setStreamSourceCalls[0].pStreamData == &mockVB);
        CHECK(mockDevice.setStreamSourceCalls[0].OffsetInBytes == 0);
        CHECK(mockDevice.setStreamSourceCalls[0].Stride == 24);

        CHECK(mockDevice.setIndicesCalls.empty());

        REQUIRE(mockDevice.drawIndexedPrimitiveCalls.size() == 1);
        CHECK(mockDevice.drawIndexedPrimitiveCalls[0].Type == D3DPT_LINELIST);
    }
}

