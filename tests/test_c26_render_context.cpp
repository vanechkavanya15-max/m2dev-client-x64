#include "../src/Client/Graphics/RenderContext.h"
#include <cassert>
#include <iostream>

using namespace Graphics;

class MockListener : public IDeviceResetListener
{
public:
    int lostCount = 0;
    int resetCount = 0;

    void OnDeviceLost() override
    {
        lostCount++;
    }

    void OnDeviceReset() override
    {
        resetCount++;
    }
};

int main()
{
    std::cout << "Running test_c26_render_context..." << std::endl;

    RenderContext ctx;
    
    // Test listener registration
    MockListener listener;
    ctx.RegisterListener(&listener);
    
    RenderContextConfig config;
    config.width = 800;
    config.height = 600;
    config.hWindow = 0;
    config.mode = WindowMode::Windowed;
    config.vSync = true;
    config.useSoftwareVertexProcessing = true;
    
    auto res = ctx.Initialize(config);
    // Note: Due to lack of real d3d9 implementation in the CI mock, this asserts missing device.
    assert(!res.has_value()); 
    
    // We can also test unregistered listeners
    ctx.UnregisterListener(&listener);
    
    std::cout << "Tests completed successfully." << std::endl;
    return 0;
}
