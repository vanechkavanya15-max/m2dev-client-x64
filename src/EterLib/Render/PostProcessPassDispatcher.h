#pragma once

#include <d3d9.h>

namespace EterLib::Render
{
    class PostProcessPassDispatcher
    {
    public:
        void Execute(LPDIRECT3DDEVICE9 dev, LPDIRECT3DTEXTURE9 source, LPDIRECT3DSURFACE9 dest, LPDIRECT3DPIXELSHADER9 effectShader);
    };
}

