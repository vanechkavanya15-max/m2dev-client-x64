#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#endif

#pragma once

#include <cstdint>
#include <array>

#ifdef TEST_MODE_DISABLE_STDAFX
// Mock D3D9 types for linux tests
typedef struct _D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
        };
        float m[4][4];
    };
} D3DMATRIX;

struct IDirect3DDevice9;
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;

struct IDirect3DDevice9 {
    virtual ~IDirect3DDevice9() {}
    virtual int SetVertexShaderConstantF(uint32_t StartRegister, const float* pConstantData, uint32_t Vector4fCount) = 0;
};
#else
// Real environment types
#ifdef _WIN32
#include <d3d9.h>
#endif
#endif

namespace EterLib::Render {

class ShaderConstantBuffer {
public:
    static constexpr uint32_t MAX_REGISTERS = 256;

    ShaderConstantBuffer();
    ~ShaderConstantBuffer() = default;

    void SetMatrix(uint32_t startRegister, const D3DMATRIX& mat);
    void SetVector4(uint32_t startRegister, float x, float y, float z, float w);
    
    void Commit(LPDIRECT3DDEVICE9 dev) noexcept;

private:
    std::array<float, MAX_REGISTERS * 4> m_buffer;
    
    uint32_t m_dirtyStart;
    uint32_t m_dirtyEnd;
    bool m_isDirty;
};

} // namespace EterLib::Render

