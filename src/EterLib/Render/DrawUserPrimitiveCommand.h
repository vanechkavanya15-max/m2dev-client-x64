#pragma once

#include <d3d9.h>

#include <cstdint>
#include <cstring>

namespace EterLib::Render
{
    struct DrawUserPrimitiveCommand
    {
        D3DPRIMITIVETYPE primitiveType;
        UINT primitiveCount;
        const void* vertexStreamZeroData;
        UINT vertexStreamZeroStride;

        void Execute(LPDIRECT3DDEVICE9 device) const noexcept
        {
            if (device && vertexStreamZeroData && primitiveCount > 0 && vertexStreamZeroStride > 0)
            {
                device->DrawPrimitiveUP(primitiveType, primitiveCount, vertexStreamZeroData, vertexStreamZeroStride);
            }
        }

        template <typename TAllocator>
        static DrawUserPrimitiveCommand* Allocate(TAllocator& allocator, D3DPRIMITIVETYPE type, UINT count, const void* data, UINT stride)
        {
            if (!data || count == 0 || stride == 0)
            {
                return nullptr;
            }

            // Allocate space for the command structure itself
            auto* cmd = static_cast<DrawUserPrimitiveCommand*>(allocator.Allocate(sizeof(DrawUserPrimitiveCommand)));
            if (!cmd)
            {
                return nullptr;
            }

            // Calculate size for vertices data and allocate
            size_t dataSize = static_cast<size_t>(count) * stride;
            
            // Depending on primitive type, the vertices count might be different than primitiveCount.
            // For safety in UP rendering, calculate real vertices count:
            UINT vertexCount = 0;
            switch (type)
            {
                case D3DPT_POINTLIST:     vertexCount = count; break;
                case D3DPT_LINELIST:      vertexCount = count * 2; break;
                case D3DPT_LINESTRIP:     vertexCount = count + 1; break;
                case D3DPT_TRIANGLELIST:  vertexCount = count * 3; break;
                case D3DPT_TRIANGLESTRIP: vertexCount = count + 2; break;
                case D3DPT_TRIANGLEFAN:   vertexCount = count + 2; break;
                default:                  vertexCount = count * 3; break; // fallback
            }

            dataSize = static_cast<size_t>(vertexCount) * stride;
            
            void* copiedData = allocator.Allocate(dataSize);
            if (!copiedData)
            {
                // Note: We leak the 'cmd' space in allocator here, but LinearFrameAllocator clears per frame so it's acceptable.
                return nullptr;
            }

            std::memcpy(copiedData, data, dataSize);

            // Setup the command
            cmd->primitiveType = type;
            cmd->primitiveCount = count;
            cmd->vertexStreamZeroData = copiedData;
            cmd->vertexStreamZeroStride = stride;

            return cmd;
        }
    };
}

