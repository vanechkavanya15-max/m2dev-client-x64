#pragma once

#ifndef _WIN32
// In linux tests, we skip StdAfx.h
#else
#include "../StdAfx.h"
#endif

#include "IRenderHardwareInterface.h"

// Forward declarations are required if the struct is not included yet.
struct IDirect3DDevice9;
struct D3DVIEWPORT9;

namespace EterLib::Render
{
    class D3D9RenderHardwareInterface : public IRenderHardwareInterface
    {
    public:
        explicit D3D9RenderHardwareInterface(IDirect3DDevice9* device)
            : m_device(device)
        {
        }

        void BeginFrame() override
        {
            if (m_device)
                m_device->BeginScene();
        }

        void EndFrame() override
        {
            if (m_device)
                m_device->EndScene();
        }

        void SetViewport(int x, int y, int w, int h) override
        {
            if (m_device)
            {
                D3DVIEWPORT9 vp;
                vp.X = x;
                vp.Y = y;
                vp.Width = w;
                vp.Height = h;
                vp.MinZ = 0.0f;
                vp.MaxZ = 1.0f;
                m_device->SetViewport(&vp);
            }
        }

        void Clear(uint32_t flags, uint32_t color, float depth) override
        {
            if (m_device)
            {
                m_device->Clear(0, nullptr, flags, color, depth, 0);
            }
        }

    private:
        IDirect3DDevice9* m_device;
    };
}

