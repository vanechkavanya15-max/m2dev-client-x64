#undef _WIN32
#include <cassert>
#include <iostream>
#include <map>
#include <vector>

// -------------------------------------------------------------------------
// Mock D3D9 Definitions
// -------------------------------------------------------------------------
typedef unsigned int DWORD;
typedef int BOOL;
#define TRUE 1
#define FALSE 0

enum D3DRENDERSTATETYPE {
    D3DRS_ZWRITEENABLE = 14,
    D3DRS_COLORWRITEENABLE = 168
};

struct IDirect3DDevice9 {
    int dummy = 0;
};
typedef IDirect3DDevice9* LPDIRECT3DDEVICE9;

struct IDirect3DPixelShader9 {
    int id;
};
typedef IDirect3DPixelShader9* LPDIRECT3DPIXELSHADER9;

// -------------------------------------------------------------------------
// Mock StateManager
// -------------------------------------------------------------------------
class MockStateManager {
public:
    std::map<DWORD, DWORD> renderStates;
    LPDIRECT3DPIXELSHADER9 currentPixelShader = nullptr;

    void SaveRenderState(DWORD state, DWORD value) {
        renderStates[state] = value;
    }

    void RestoreRenderState(DWORD state) {
        renderStates.erase(state);
    }

    void GetPixelShader(LPDIRECT3DPIXELSHADER9* shader) {
        if (shader) *shader = currentPixelShader;
    }

    void SetPixelShader(LPDIRECT3DPIXELSHADER9 shader) {
        currentPixelShader = shader;
    }
};

MockStateManager STATEMANAGER;

// -------------------------------------------------------------------------
// Mock Header Includes
// -------------------------------------------------------------------------
namespace EterLib::Render
{
    class ColorWriteScope
    {
    public:
        explicit ColorWriteScope(DWORD colorWriteEnableFlags)
        {
            STATEMANAGER.SaveRenderState(D3DRS_COLORWRITEENABLE, colorWriteEnableFlags);
        }

        ~ColorWriteScope()
        {
            STATEMANAGER.RestoreRenderState(D3DRS_COLORWRITEENABLE);
        }
    };

    class PixelShaderScope
    {
    public:
        explicit PixelShaderScope(LPDIRECT3DPIXELSHADER9 newShader)
            : m_originalShader(nullptr), m_stateChanged(false)
        {
            LPDIRECT3DPIXELSHADER9 currentShader = nullptr;
            STATEMANAGER.GetPixelShader(&currentShader);

            if (currentShader != newShader)
            {
                m_originalShader = currentShader;
                m_stateChanged = true;
                STATEMANAGER.SetPixelShader(newShader);
            }
        }

        ~PixelShaderScope()
        {
            if (m_stateChanged)
            {
                STATEMANAGER.SetPixelShader(m_originalShader);
            }
        }

    private:
        LPDIRECT3DPIXELSHADER9 m_originalShader;
        bool m_stateChanged;
    };
} // namespace EterLib::Render



#include <optional>

namespace EterLib::Render
{
    /**
     * @brief A RAII-based scope guard for managing D3DRS_ZWRITEENABLE.
     */
    class ZBufferScope
    {
    public:
        ZBufferScope()
        {
            STATEMANAGER.SaveRenderState(D3DRS_ZWRITEENABLE, TRUE);
        }

        ~ZBufferScope()
        {
            STATEMANAGER.RestoreRenderState(D3DRS_ZWRITEENABLE);
        }

        ZBufferScope(const ZBufferScope&) = delete;
        ZBufferScope& operator=(const ZBufferScope&) = delete;
        ZBufferScope(ZBufferScope&&) = delete;
        ZBufferScope& operator=(ZBufferScope&&) = delete;
    };

    /**
     * @brief Manages D3D9 pipeline configuration for an early Z-pass (Depth Prepass).
     */
    class DepthPrepassDispatcher
    {
    public:
        DepthPrepassDispatcher() = default;
        ~DepthPrepassDispatcher() = default;

        /**
         * @brief Begins the depth prepass, configuring D3D9 states.
         * 
         * @param dev The active Direct3D device.
         */
        void BeginPass(LPDIRECT3DDEVICE9 dev) noexcept;

        /**
         * @brief Ends the depth prepass, restoring prior D3D9 states.
         * 
         * @param dev The active Direct3D device.
         */
        void EndPass(LPDIRECT3DDEVICE9 dev) noexcept;

    private:
        std::optional<ColorWriteScope> m_colorWriteScope;
        std::optional<ZBufferScope> m_zBufferScope;
        std::optional<PixelShaderScope> m_pixelShaderScope;
    };
} // namespace EterLib::Render

namespace EterLib::Render
{
    void DepthPrepassDispatcher::BeginPass(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (!dev)
        {
            return;
        }

        // 1. Disable color writes (RGBA)
        m_colorWriteScope.emplace(0);

        // 2. Enable Z-buffer writing
        m_zBufferScope.emplace();

        // 3. Disable pixel shaders (null pixel shader for pure depth pass)
        m_pixelShaderScope.emplace(nullptr);
    }

    void DepthPrepassDispatcher::EndPass(LPDIRECT3DDEVICE9 dev) noexcept
    {
        if (!dev)
        {
            return;
        }

        // Restore scopes in reverse order of creation
        m_pixelShaderScope.reset();
        m_zBufferScope.reset();
        m_colorWriteScope.reset();
    }
} // namespace EterLib::Render

// -------------------------------------------------------------------------
// Tests
// -------------------------------------------------------------------------
void test_null_device()
{
    EterLib::Render::DepthPrepassDispatcher dispatcher;
    dispatcher.BeginPass(nullptr);
    assert(STATEMANAGER.renderStates.empty());
    assert(STATEMANAGER.currentPixelShader == nullptr);
    dispatcher.EndPass(nullptr);
    std::cout << "[OK] test_null_device\n";
}

void test_valid_device_pass()
{
    IDirect3DDevice9 dummyDevice;
    IDirect3DPixelShader9 dummyShader{ 42 };

    // Setup initial state
    STATEMANAGER.currentPixelShader = &dummyShader;
    STATEMANAGER.renderStates.clear();

    EterLib::Render::DepthPrepassDispatcher dispatcher;

    // Begin Pass
    dispatcher.BeginPass(&dummyDevice);
    
    // Verify Color Write is 0
    assert(STATEMANAGER.renderStates.count(D3DRS_COLORWRITEENABLE) > 0);
    assert(STATEMANAGER.renderStates[D3DRS_COLORWRITEENABLE] == 0);
    
    // Verify Z-Buffer is enabled (TRUE)
    assert(STATEMANAGER.renderStates.count(D3DRS_ZWRITEENABLE) > 0);
    assert(STATEMANAGER.renderStates[D3DRS_ZWRITEENABLE] == TRUE);

    // Verify Pixel Shader is Null
    assert(STATEMANAGER.currentPixelShader == nullptr);

    // End Pass
    dispatcher.EndPass(&dummyDevice);

    // Verify scopes restored state (our mock removes them from map on restore)
    assert(STATEMANAGER.renderStates.count(D3DRS_COLORWRITEENABLE) == 0);
    assert(STATEMANAGER.renderStates.count(D3DRS_ZWRITEENABLE) == 0);
    
    // Verify Pixel Shader is restored
    assert(STATEMANAGER.currentPixelShader == &dummyShader);

    std::cout << "[OK] test_valid_device_pass\n";
}

int main()
{
    std::cout << "Running DepthPrepassDispatcher Tests...\n";
    test_null_device();
    test_valid_device_pass();
    std::cout << "All tests passed!\n";
    return 0;
}

