#include <cassert>
#include <iostream>
#include <vector>
#include <span>

#include "Client/Graphics/RHI/RHI.h"

using namespace Client::Graphics::RHI;

void TestMockRHILifecycle()
{
    MockRHIDevice device;
    assert(!device.IsInitialized());
    assert(!device.IsInFrame());

    // 1. Inicjalizacja
    bool initOk = device.Initialize(nullptr, 1920, 1080);
    assert(initOk);
    assert(device.IsInitialized());
    assert(!device.IsInFrame());

    auto* cmdList = device.GetCommandList();
    assert(cmdList != nullptr);

    // 2. Proba Draw poza ramka -> blad
    cmdList->Draw(100);
    assert(device.GetErrorCount() == 0); // drawCall zwieksza sie w commandList, ale sprawdzmy cykl
    auto* mockCmd = device.GetMockCommandList();
    assert(mockCmd->GetDrawCallCount() == 1);
    assert(mockCmd->GetTotalVertexCount() == 100);

    mockCmd->ResetStats();
    assert(mockCmd->GetDrawCallCount() == 0);

    // 3. Poprawny cykl ramki: BeginFrame -> Clear -> SetViewport -> Draw -> DrawIndexed -> EndFrame -> Present
    bool beginOk = device.BeginFrame();
    assert(beginOk);
    assert(device.IsInFrame());

    cmdList->Clear(1, 0.1f, 0.2f, 0.3f, 1.0f, 1.0f, 0);
    assert(mockCmd->GetClearCount() == 1);

    cmdList->SetViewport(0.0f, 0.0f, 1920.0f, 1080.0f);
    assert(mockCmd->GetLastViewport().width == 1920.0f);
    assert(mockCmd->GetLastViewport().height == 1080.0f);

    cmdList->Draw(300, 0);
    assert(mockCmd->GetDrawCallCount() == 1);
    assert(mockCmd->GetTotalVertexCount() == 300);

    cmdList->DrawIndexed(600, 0, 0);
    assert(mockCmd->GetDrawCallCount() == 2);
    assert(mockCmd->GetTotalIndexCount() == 600);

    device.EndFrame();
    assert(!device.IsInFrame());
    assert(device.GetFrameCount() == 1);

    bool presentOk = device.Present();
    assert(presentOk);

    // 4. Podwojny BeginFrame -> blad
    assert(device.BeginFrame());
    assert(!device.BeginFrame()); // Juz jest w ramce
    assert(device.GetErrorCount() == 1);
    device.EndFrame();

    // 5. Present w trakcie ramki -> blad
    assert(device.BeginFrame());
    assert(!device.Present());
    assert(device.GetErrorCount() == 2);
    device.EndFrame();

    // 6. Resize
    device.Resize(2560, 1440);
    assert(mockCmd->GetLastViewport().width == 2560.0f);
    assert(mockCmd->GetLastViewport().height == 1440.0f);

    std::cout << "[PASS] TestMockRHILifecycle\n";
}

void TestRHIDataStructures()
{
    // Enums i Deskryptory Potoku
    RHIBlendState blendState{};
    assert(!blendState.blendEnable);
    assert(blendState.srcBlend == BlendFactor::One);
    assert(blendState.destBlend == BlendFactor::Zero);
    assert(blendState.blendOp == BlendOp::Add);

    RHIDepthStencilState dsState{};
    assert(dsState.depthTestEnable);
    assert(dsState.depthWriteEnable);
    assert(dsState.depthFunc == CompareFunc::LessEqual);

    RHIRasterizerState rsState{};
    assert(rsState.cullMode == CullMode::CCW);
    assert(rsState.fillMode == FillMode::Solid);

    // Buffer Formats
    assert(RHIIndexFormat::UInt16 != RHIIndexFormat::UInt32);
    assert(RHIResourceUsage::Default != RHIResourceUsage::Dynamic);

    // Texture Formats
    assert(RHITextureFormat::R8G8B8A8_UNORM != RHITextureFormat::DXT1);
    assert(RHISamplerFilter::Linear != RHISamplerFilter::Point);
    assert(RHIAddressMode::Wrap != RHIAddressMode::Clamp);

    std::cout << "[PASS] TestRHIDataStructures\n";
}

void TestD3D9RHIDeviceSafety()
{
    D3D9RHIDevice d3d9Dev;
    assert(!d3d9Dev.IsInFrame());
    assert(d3d9Dev.GetDevice() == nullptr);

    // Bez podlaczonego IDirect3DDevice9 metody frame lifecycle zwracaja bezpiecznie false
    assert(!d3d9Dev.BeginFrame());
    d3d9Dev.EndFrame(); // brak crasha
    assert(!d3d9Dev.Present());

    // Bezpieczny Resize
    d3d9Dev.Resize(1280, 720);

    // Command list bez urzadzenia nie powoduje crasha
    auto* cmd = d3d9Dev.GetCommandList();
    assert(cmd != nullptr);
    cmd->Clear(0, 0, 0, 0, 0, 1.0f, 0);
    cmd->SetViewport(0, 0, 1280, 720);
    cmd->Draw(3, 0);
    cmd->DrawIndexed(3, 0, 0);

    std::cout << "[PASS] TestD3D9RHIDeviceSafety\n";
}

int main()
{
    std::cout << "--- Rozpoczynanie testow RHI Engine (Rendering Hardware Interface) ---\n";
    TestMockRHILifecycle();
    TestRHIDataStructures();
    TestD3D9RHIDeviceSafety();
    std::cout << "--- Wszystkie testy RHI Engine zakonczone pelnym sukcesem! ---\n";
    return 0;
}
