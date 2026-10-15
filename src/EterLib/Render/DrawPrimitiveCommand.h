#pragma once

#ifndef TEST_MOCK_D3D9
#include <d3d9.h>
#endif

namespace EterLib::Render
{
    struct DrawPrimitiveCommand
    {
        D3DPRIMITIVETYPE primitiveType;
        UINT startVertex;
        UINT primitiveCount;
        LPDIRECT3DVERTEXBUFFER9 vertexBuffer;
        UINT stride;

        void Execute(LPDIRECT3DDEVICE9 device) const noexcept
        {
            if (!device || !vertexBuffer)
                return;

            device->SetStreamSource(0, vertexBuffer, 0, stride);
            device->DrawPrimitive(primitiveType, startVertex, primitiveCount);
        }
    };
}

