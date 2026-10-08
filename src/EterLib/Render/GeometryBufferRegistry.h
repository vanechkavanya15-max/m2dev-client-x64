#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "../../StdAfx.h"
#include <d3d9.h>
#else
#include <cstdint>

typedef unsigned int UINT;
typedef long HRESULT;
#define S_OK 0
#define FAILED(hr) (((HRESULT)(hr)) < 0)

struct IDirect3DVertexBuffer9 {};
typedef IDirect3DVertexBuffer9* LPDIRECT3DVERTEXBUFFER9;

struct IDirect3DIndexBuffer9 {};
typedef IDirect3DIndexBuffer9* LPDIRECT3DINDEXBUFFER9;

struct IDirect3DDevice9 {
    virtual HRESULT SetStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT OffsetInBytes, UINT Stride) = 0;
    virtual HRESULT SetIndices(LPDIRECT3DINDEXBUFFER9 pIndexData) = 0;
    virtual ~IDirect3DDevice9() = default;
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;
#endif

#include <array>
#include <cstddef>

namespace EterLib::Render
{
    class GeometryBufferRegistry
    {
    public:
        GeometryBufferRegistry() noexcept;
        ~GeometryBufferRegistry() noexcept = default;

        bool BindVertexBuffer(LPDIRECT3DDEVICE9 dev, UINT stream, LPDIRECT3DVERTEXBUFFER9 vb, UINT offset, UINT stride) noexcept;
        bool BindIndexBuffer(LPDIRECT3DDEVICE9 dev, LPDIRECT3DINDEXBUFFER9 ib) noexcept;
        void Invalidate() noexcept;

    private:
        struct StreamState
        {
            LPDIRECT3DVERTEXBUFFER9 vb{nullptr};
            UINT offset{0};
            UINT stride{0};
        };

        static constexpr std::size_t MAX_STREAMS = 16;
        std::array<StreamState, MAX_STREAMS> m_streams{};
        LPDIRECT3DINDEXBUFFER9 m_currentIndexBuffer{nullptr};
    };
}

