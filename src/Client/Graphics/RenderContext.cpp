#include "RenderContext.h"
#include "EterBase/ModernLogger.h"

namespace Graphics
{
    RenderContext::RenderContext()
        : m_pD3D(nullptr)
        , m_pDevice(nullptr)
        , m_deviceLost(false)
        , m_config{}
    {
    }

    RenderContext::~RenderContext()
    {
        if (m_pDevice)
        {
            m_pDevice->Release();
            m_pDevice = nullptr;
        }

        if (m_pD3D)
        {
            m_pD3D->Release();
            m_pD3D = nullptr;
        }
    }

    void RenderContext::BuildPresentParameters(D3DPRESENT_PARAMETERS& outParams) const
    {
        ZeroMemory(&outParams, sizeof(outParams));

        outParams.BackBufferWidth = m_config.width;
        outParams.BackBufferHeight = m_config.height;
        outParams.BackBufferFormat = D3DFMT_X8R8G8B8; // Standard 32-bit format
        outParams.BackBufferCount = 1;
        outParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
        outParams.hDeviceWindow = m_config.hWindow;
        
        switch (m_config.mode)
        {
        case WindowMode::Windowed:
        case WindowMode::Borderless:
            outParams.Windowed = TRUE;
            outParams.FullScreen_RefreshRateInHz = 0;
            break;
        case WindowMode::Fullscreen:
            outParams.Windowed = FALSE;
            outParams.FullScreen_RefreshRateInHz = D3DPRESENT_INTERVAL_DEFAULT; // Default refresh rate
            break;
        }

        outParams.EnableAutoDepthStencil = TRUE;
        outParams.AutoDepthStencilFormat = D3DFMT_D24S8; // 24-bit depth, 8-bit stencil
        
        if (m_config.vSync)
        {
            outParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        }
        else
        {
            outParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        }
    }

    EterBase::VoidResult<std::string_view> RenderContext::Initialize(const RenderContextConfig& config)
    {
        if (m_pDevice || m_pD3D)
        {
            EterBase::ModernLogger::Error("RenderContext is already initialized.");
            return EterBase::MakeError("RenderContext is already initialized");
        }

        m_config = config;

        m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
        if (!m_pD3D)
        {
            EterBase::ModernLogger::Error("Failed to create Direct3D9 instance.");
            return EterBase::MakeError("Direct3DCreate9 failed");
        }

        D3DPRESENT_PARAMETERS d3dpp;
        BuildPresentParameters(d3dpp);

        DWORD behaviorFlags = D3DCREATE_MULTITHREADED;
        
        if (m_config.useSoftwareVertexProcessing)
        {
            behaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
        else
        {
            behaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
        }

        // Try creating device with preferred flags
        HRESULT hr = m_pD3D->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            m_config.hWindow,
            behaviorFlags,
            &d3dpp,
            &m_pDevice);

        if (FAILED(hr) && !m_config.useSoftwareVertexProcessing)
        {
            EterBase::ModernLogger::Info("Hardware vertex processing failed. Trying mixed vertex processing.");
            
            behaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            behaviorFlags |= D3DCREATE_MIXED_VERTEXPROCESSING;
            
            hr = m_pD3D->CreateDevice(
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_HAL,
                m_config.hWindow,
                behaviorFlags,
                &d3dpp,
                &m_pDevice);
                
            if (FAILED(hr))
            {
                EterBase::ModernLogger::Info("Mixed vertex processing failed. Trying software vertex processing.");
                
                behaviorFlags &= ~D3DCREATE_MIXED_VERTEXPROCESSING;
                behaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
                
                hr = m_pD3D->CreateDevice(
                    D3DADAPTER_DEFAULT,
                    D3DDEVTYPE_HAL,
                    m_config.hWindow,
                    behaviorFlags,
                    &d3dpp,
                    &m_pDevice);
            }
        }

        if (FAILED(hr))
        {
            EterBase::ModernLogger::Error("Failed to create Direct3D9 Device.");
            if (m_pD3D)
            {
                m_pD3D->Release();
                m_pD3D = nullptr;
            }
            return EterBase::MakeError("IDirect3D9::CreateDevice failed");
        }

        m_deviceLost = false;
        EterBase::ModernLogger::Info("RenderContext initialized successfully.");
        return {};
    }
}

namespace Graphics
{
    void RenderContext::RegisterListener(IDeviceResetListener* pListener)
    {
        if (!pListener)
            return;
            
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        m_listeners.push_back(pListener);
    }

    void RenderContext::UnregisterListener(IDeviceResetListener* pListener)
    {
        if (!pListener)
            return;
            
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        std::erase(m_listeners, pListener);
    }

    bool RenderContext::HandleDeviceLost()
    {
        if (!m_pDevice)
            return false;

        HRESULT hr = m_pDevice->TestCooperativeLevel();
        if (hr == D3DERR_DEVICELOST)
        {
            // The device is still lost, cannot reset yet.
            Sleep(50); // Pause briefly so we don't spin 100% CPU.
            return false;
        }
        else if (hr == D3DERR_DEVICENOTRESET)
        {
            // The device is ready to be reset.
            EterBase::ModernLogger::Info("Device is ready to reset. Initiating recovery...");

            // Notify listeners to release D3DPOOL_DEFAULT resources
            std::vector<IDeviceResetListener*> listenersCopy;
            {
                std::lock_guard<std::mutex> lock(m_listenerMutex);
                listenersCopy = m_listeners;
            }
            
            for (auto* listener : listenersCopy)
            {
                listener->OnDeviceLost();
            }

            D3DPRESENT_PARAMETERS d3dpp;
            BuildPresentParameters(d3dpp);

            hr = m_pDevice->Reset(&d3dpp);
            if (FAILED(hr))
            {
                EterBase::ModernLogger::Error("Failed to reset Direct3D device. hr={}", (int)hr);
                Sleep(50);
                return false;
            }

            // Notify listeners to recreate resources
            for (auto* listener : listenersCopy)
            {
                listener->OnDeviceReset();
            }

            EterBase::ModernLogger::Info("Device reset successfully.");
            m_deviceLost = false;
            return true;
        }

        // Catch-all failure
        if (FAILED(hr))
        {
            return false;
        }

        m_deviceLost = false;
        return true;
    }

    bool RenderContext::Present()
    {
        if (!m_pDevice)
            return false;

        if (m_deviceLost)
        {
            return HandleDeviceLost();
        }

        HRESULT hr = m_pDevice->Present(nullptr, nullptr, 0, nullptr);
        if (hr == D3DERR_DEVICELOST)
        {
            EterBase::ModernLogger::Error("Device lost detected during Present().");
            m_deviceLost = true;
            return false;
        }

        return SUCCEEDED(hr);
    }
}
