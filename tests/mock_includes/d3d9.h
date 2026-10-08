#pragma once
 
#ifndef D3DPT_TRIANGLELIST
typedef unsigned int UINT;
typedef int INT;

typedef enum _D3DPRIMITIVETYPE {
    D3DPT_TRIANGLELIST = 4,
} D3DPRIMITIVETYPE;

struct IDirect3DVertexBuffer9 {};
typedef IDirect3DVertexBuffer9* LPDIRECT3DVERTEXBUFFER9;

struct IDirect3DIndexBuffer9 {};
typedef IDirect3DIndexBuffer9* LPDIRECT3DINDEXBUFFER9;

struct IDirect3DDevice9 {
    void SetStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT OffsetInBytes, UINT Stride) {}
    void SetIndices(LPDIRECT3DINDEXBUFFER9 pIndexData) {}
    void DrawIndexedPrimitive(D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {}
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;
#endif

