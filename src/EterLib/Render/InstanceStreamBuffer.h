#pragma once

#include <span>
#include <cstdint>
#include <d3d9.h>

namespace EterLib::Render
{
    class InstanceStreamBuffer
    {
    public:
        struct InstanceData
        {
            D3DMATRIX world;
            uint32_t tintColor;
            float lodFactor;
        };

        InstanceStreamBuffer();
        ~InstanceStreamBuffer();

        bool Initialize(LPDIRECT3DDEVICE9 dev, size_t maxInstances);
        void UpdateInstances(std::span<const InstanceData> instances);
        void Bind(LPDIRECT3DDEVICE9 dev, uint32_t streamIndex = 1);

    private:
        LPDIRECT3DVERTEXBUFFER9 m_pVB;
        size_t m_maxInstances;
        size_t m_currentInstances;
    };
}

