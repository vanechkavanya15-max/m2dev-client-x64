#include "ShaderConstantBuffer.h"
#include <algorithm>
#include <cstring>

namespace EterLib::Render {

ShaderConstantBuffer::ShaderConstantBuffer() 
    : m_dirtyStart(MAX_REGISTERS), m_dirtyEnd(0), m_isDirty(false) {
    m_buffer.fill(0.0f);
}

void ShaderConstantBuffer::SetMatrix(uint32_t startRegister, const D3DMATRIX& mat) {
    if (startRegister + 4 > MAX_REGISTERS) {
        return;
    }
    
    // Check if the matrix actually changed
    if (std::memcmp(&m_buffer[startRegister * 4], &mat.m[0][0], sizeof(float) * 16) == 0) {
        return;
    }

    std::memcpy(&m_buffer[startRegister * 4], &mat.m[0][0], sizeof(float) * 16);
    
    m_isDirty = true;
    m_dirtyStart = std::min(m_dirtyStart, startRegister);
    m_dirtyEnd = std::max(m_dirtyEnd, startRegister + 4);
}

void ShaderConstantBuffer::SetVector4(uint32_t startRegister, float x, float y, float z, float w) {
    if (startRegister >= MAX_REGISTERS) {
        return;
    }

    float* target = &m_buffer[startRegister * 4];
    
    // Check if the vector actually changed
    if (target[0] == x && target[1] == y && target[2] == z && target[3] == w) {
        return;
    }

    target[0] = x;
    target[1] = y;
    target[2] = z;
    target[3] = w;

    m_isDirty = true;
    m_dirtyStart = std::min(m_dirtyStart, startRegister);
    m_dirtyEnd = std::max(m_dirtyEnd, startRegister + 1);
}

void ShaderConstantBuffer::Commit(LPDIRECT3DDEVICE9 dev) noexcept {
    if (!m_isDirty || !dev) {
        return;
    }

    uint32_t count = m_dirtyEnd - m_dirtyStart;
    
    dev->SetVertexShaderConstantF(m_dirtyStart, &m_buffer[m_dirtyStart * 4], count);

    // Reset dirty tracking
    m_isDirty = false;
    m_dirtyStart = MAX_REGISTERS;
    m_dirtyEnd = 0;
}

} // namespace EterLib::Render

