#include <iostream>
#include <cstdint>
#include <cassert>

// Testing DirectX COM interfaces in isolated unit tests is difficult because of their pure virtual nature.
// We test null safety. For full coverage of the stack, integration tests with a graphics device are necessary.

#include <d3d9.h>
#include "../src/EterLib/Render/RenderTargetManager.h"

int main() {
    EterLib::Render::RenderTargetManager rtm;
    
    // Test null device safety
    LPDIRECT3DTEXTURE9 tex = rtm.CreateRenderTarget(nullptr, 256, 256, D3DFMT_A8R8G8B8);
    assert(tex == nullptr);
    
    rtm.PushRenderTarget(nullptr, nullptr);
    rtm.PopRenderTarget(nullptr);
    
    std::cout << "All null-safety tests passed.\n";
    return 0;
}

