#pragma once
#include "../StdAfx.h"

namespace EterLib::Render
{
    enum class CullMode : int
    {
        None = 1, // D3DCULL_NONE
        Clockwise = 2, // D3DCULL_CW
        CounterClockwise = 3 // D3DCULL_CCW
    };

    enum class FillMode : int
    {
        Solid = 3, // D3DFILL_SOLID
        Wireframe = 2 // D3DFILL_WIREFRAME
    };

    class RasterizerScope
    {
    public:
        RasterizerScope(IDirect3DDevice9* device, CullMode cull, FillMode fill, bool scissorEnable)
            : m_device(device)
        {
            if (m_device)
            {
                m_device->GetRenderState(D3DRS_CULLMODE, &m_oldCullMode);
                m_device->GetRenderState(D3DRS_FILLMODE, &m_oldFillMode);
                m_device->GetRenderState(D3DRS_SCISSORTESTENABLE, &m_oldScissorEnable);

                m_device->SetRenderState(D3DRS_CULLMODE, static_cast<DWORD>(cull));
                m_device->SetRenderState(D3DRS_FILLMODE, static_cast<DWORD>(fill));
                m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, scissorEnable ? 1 : 0);
            }
        }

        ~RasterizerScope()
        {
            if (m_device)
            {
                m_device->SetRenderState(D3DRS_CULLMODE, m_oldCullMode);
                m_device->SetRenderState(D3DRS_FILLMODE, m_oldFillMode);
                m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, m_oldScissorEnable);
            }
        }

        // Zablokowanie kopiowania i przenoszenia, to klasa RAII.
        RasterizerScope(const RasterizerScope&) = delete;
        RasterizerScope& operator=(const RasterizerScope&) = delete;
        RasterizerScope(RasterizerScope&&) = delete;
        RasterizerScope& operator=(RasterizerScope&&) = delete;

    private:
        IDirect3DDevice9* m_device;
        DWORD m_oldCullMode{ 0 };
        DWORD m_oldFillMode{ 0 };
        DWORD m_oldScissorEnable{ 0 };
    };
}
