#pragma once

#include <cstdint>

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

namespace Client::Graphics::Debug
{

#pragma pack(push, 1)
struct DebugVertex
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    uint32_t color = 0xFFFFFFFF; // ARGB / D3DCOLOR format

#if defined(_WIN32) || defined(_WIN64)
    static constexpr DWORD FVF = (D3DFVF_XYZ | D3DFVF_DIFFUSE);
#else
    static constexpr uint32_t FVF = 0x042;
#endif
};
#pragma pack(pop)

static_assert(sizeof(DebugVertex) == 16, "DebugVertex must be exactly 16 bytes for D3DFVF_XYZ | D3DFVF_DIFFUSE");

constexpr uint32_t D3DFVF_DEBUG_VERTEX = 0x042; // D3DFVF_XYZ (0x002) | D3DFVF_DIFFUSE (0x040)

} // namespace Client::Graphics::Debug
