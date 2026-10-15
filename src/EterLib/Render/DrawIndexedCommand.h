#ifndef ETERLIB_RENDER_DRAWINDEXEDCOMMAND_H
#define ETERLIB_RENDER_DRAWINDEXEDCOMMAND_H

#include <d3d9.h>

namespace EterLib::Render
{
    /**
     * @brief Encapsulates a DirectX 9 DrawIndexedPrimitive call with its associated state.
     */
    struct DrawIndexedCommand
    {
        D3DPRIMITIVETYPE primitiveType;
        INT baseVertexIndex;
        UINT minVertexIndex;
        UINT numVertices;
        UINT startIndex;
        UINT primitiveCount;
        LPDIRECT3DVERTEXBUFFER9 vertexBuffer;
        LPDIRECT3DINDEXBUFFER9 indexBuffer;
        UINT stride;

        /**
         * @brief Executes the draw command on the specified Direct3D device.
         * @param device The Direct3D 9 device to execute the command on.
         */
        void Execute(LPDIRECT3DDEVICE9 device) const noexcept
        {
            if (!device)
                return;

            if (vertexBuffer)
            {
                device->SetStreamSource(0, vertexBuffer, 0, stride);
            }

            if (indexBuffer)
            {
                device->SetIndices(indexBuffer);
            }

            device->DrawIndexedPrimitive(
                primitiveType,
                baseVertexIndex,
                minVertexIndex,
                numVertices,
                startIndex,
                primitiveCount
            );
        }
    };
} // namespace EterLib::Render

#endif // ETERLIB_RENDER_DRAWINDEXEDCOMMAND_H

