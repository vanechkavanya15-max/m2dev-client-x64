#pragma once

#include "../StdAfx.h"
#include "../StateManager.h"

namespace EterLib::Render
{
    class IndexBufferScope
    {
    public:
        IndexBufferScope(LPDIRECT3DINDEXBUFFER9 pIndexData, UINT BaseVertexIndex)
        {
            STATEMANAGER.SaveIndices(pIndexData, BaseVertexIndex);
        }

        ~IndexBufferScope()
        {
            STATEMANAGER.RestoreIndices();
        }

        // Prevent copying and assignment
        IndexBufferScope(const IndexBufferScope&) = delete;
        IndexBufferScope& operator=(const IndexBufferScope&) = delete;
        
        // Prevent moving
        IndexBufferScope(IndexBufferScope&&) = delete;
        IndexBufferScope& operator=(IndexBufferScope&&) = delete;
    };
}
