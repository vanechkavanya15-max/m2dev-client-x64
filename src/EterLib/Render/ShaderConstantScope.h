#pragma once

#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StdAfx.h"
#endif

#include <vector>
#include <cstdint>
#include <stdexcept>

// Forward declaration of CStateManager
class CStateManager;

namespace EterLib::Render
{
    /**
     * @brief RAII scope for managing DirectX 9 Shader Constants
     * 
     * Automatically retrieves and saves the original values for a specified range
     * of shader constants (Vertex or Pixel shader), sets the new values upon construction,
     * and restores the original values upon destruction.
     * 
     * Useful for temporary shader parameters such as mob highlighting or one-off
     * visual effects during rendering loops without permanently altering global state.
     */
    class ShaderConstantScope
    {
    public:
        enum class ShaderType
        {
            Vertex,
            Pixel
        };

        /**
         * @brief Constructs a ShaderConstantScope.
         * 
         * @param stateManager  Pointer to the CStateManager instance (typically STATEMANAGER).
         *                      Passed as a pointer to avoid direct dependency coupling if needed.
         * @param type          Whether these are vertex or pixel shader constants.
         * @param startRegister The first register index to modify (e.g., 0).
         * @param pConstantData Pointer to an array of 4D floats representing the new constants.
         * @param registerCount The number of registers to set. Each register is 4 floats (16 bytes).
         */
        ShaderConstantScope(CStateManager* stateManager, ShaderType type, uint32_t startRegister, const void* pConstantData, uint32_t registerCount);

        ~ShaderConstantScope();

        // Prevent copying to maintain strict RAII semantics
        ShaderConstantScope(const ShaderConstantScope&) = delete;
        ShaderConstantScope& operator=(const ShaderConstantScope&) = delete;

        // Prevent moving for simplicity, unless specifically required
        ShaderConstantScope(ShaderConstantScope&&) = delete;
        ShaderConstantScope& operator=(ShaderConstantScope&&) = delete;

    private:
        CStateManager* m_stateManager;
        ShaderType m_type;
        uint32_t m_startRegister;
        uint32_t m_registerCount;
        
        // We use a vector of floats, where every 4 floats correspond to 1 register.
        std::vector<float> m_originalConstants;
    };

} // namespace EterLib::Render

#ifdef _WIN32
#ifndef TEST_MODE_DISABLE_STDAFX
#include "../StateManager.h"
#endif
#endif // _WIN32

namespace EterLib::Render
{
#ifdef _WIN32

    inline ShaderConstantScope::ShaderConstantScope(CStateManager* stateManager, ShaderType type, uint32_t startRegister, const void* pConstantData, uint32_t registerCount)
        : m_stateManager(stateManager), m_type(type), m_startRegister(startRegister), m_registerCount(registerCount)
    {
        if (!m_stateManager)
        {
            throw std::invalid_argument("ShaderConstantScope: stateManager cannot be null");
        }
        if (m_registerCount == 0)
        {
            return;
        }

        // Allocate space for the original constants: registerCount * 4 floats per register
        m_originalConstants.resize(m_registerCount * 4);

        LPDIRECT3DDEVICE9EX pDevice = m_stateManager->GetDevice();
        if (!pDevice)
        {
            throw std::runtime_error("ShaderConstantScope: D3D device is null");
        }

        HRESULT hr = E_FAIL;

        // 1. Save original constants
        if (m_type == ShaderType::Vertex)
        {
            hr = pDevice->GetVertexShaderConstantF(m_startRegister, m_originalConstants.data(), m_registerCount);
        }
        else if (m_type == ShaderType::Pixel)
        {
            hr = pDevice->GetPixelShaderConstantF(m_startRegister, m_originalConstants.data(), m_registerCount);
        }

        if (FAILED(hr))
        {
            // If we fail to get the constants, don't set them and clear our saved state
            // so the destructor knows not to restore.
            m_originalConstants.clear();
            return;
        }

        // 2. Set new constants
        if (m_type == ShaderType::Vertex)
        {
            m_stateManager->SetVertexShaderConstant(m_startRegister, pConstantData, m_registerCount);
        }
        else if (m_type == ShaderType::Pixel)
        {
            m_stateManager->SetPixelShaderConstant(m_startRegister, pConstantData, m_registerCount);
        }
    }

    inline ShaderConstantScope::~ShaderConstantScope()
    {
        if (m_originalConstants.empty() || m_registerCount == 0 || !m_stateManager)
        {
            return;
        }

        // Restore original constants
        if (m_type == ShaderType::Vertex)
        {
            m_stateManager->SetVertexShaderConstant(m_startRegister, m_originalConstants.data(), m_registerCount);
        }
        else if (m_type == ShaderType::Pixel)
        {
            m_stateManager->SetPixelShaderConstant(m_startRegister, m_originalConstants.data(), m_registerCount);
        }
    }

#else // Linux Mock Fallback

    inline ShaderConstantScope::ShaderConstantScope(CStateManager* stateManager, ShaderType type, uint32_t startRegister, const void* pConstantData, uint32_t registerCount)
        : m_stateManager(stateManager), m_type(type), m_startRegister(startRegister), m_registerCount(registerCount)
    {
        if (!m_stateManager)
        {
            throw std::invalid_argument("ShaderConstantScope: stateManager cannot be null");
        }
        if (m_registerCount == 0)
        {
            return;
        }
        m_originalConstants.resize(m_registerCount * 4, 0.0f); // Mock save
    }

    inline ShaderConstantScope::~ShaderConstantScope()
    {
    }

#endif // _WIN32
} // namespace EterLib::Render
