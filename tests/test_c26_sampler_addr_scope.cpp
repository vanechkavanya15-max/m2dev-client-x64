#include <iostream>
#include <cassert>
#include <unordered_map>
#include <vector>

// Dummy types and STATEMANAGER mock to support standalone compilation
typedef unsigned int DWORD;

enum D3DTEXTUREADDRESS {
    D3DTADDRESS_WRAP = 1,
    D3DTADDRESS_MIRROR = 2,
    D3DTADDRESS_CLAMP = 3,
    D3DTADDRESS_BORDER = 4,
    D3DTADDRESS_MIRRORONCE = 5,
};

enum D3DSAMPLERSTATETYPE {
    D3DSAMP_ADDRESSU = 1,
    D3DSAMP_ADDRESSV = 2,
    D3DSAMP_ADDRESSW = 3,
};

// Mock for STATEMANAGER
struct MockStateManager {
    std::unordered_map<DWORD, std::unordered_map<DWORD, DWORD>> samplerStates;
    int setSamplerStateCalls = 0;
    int getSamplerStateCalls = 0;

    void SetSamplerState(DWORD dwStage, D3DSAMPLERSTATETYPE Type, DWORD dwValue) {
        samplerStates[dwStage][Type] = dwValue;
        setSamplerStateCalls++;
    }

    void GetSamplerState(DWORD dwStage, D3DSAMPLERSTATETYPE Type, DWORD* pdwValue) {
        // Initialize to 0 if not set, to simulate D3D behavior
        if (samplerStates[dwStage].find(Type) == samplerStates[dwStage].end()) {
            samplerStates[dwStage][Type] = 0;
        }
        *pdwValue = samplerStates[dwStage][Type];
        getSamplerStateCalls++;
    }

    void ResetCounters() {
        setSamplerStateCalls = 0;
        getSamplerStateCalls = 0;
    }
};

// Global instance of the mock
MockStateManager g_mockStateManager;

// Define STATEMANAGER macro to use the mock
#define STATEMANAGER g_mockStateManager

// Include the actual implementation
#include "../src/EterLib/Render/SamplerAddressScope.h"

using namespace EterLib::Render;

void TestSamplerAddressScope_Wrap()
{
    STATEMANAGER.ResetCounters();
    STATEMANAGER.samplerStates.clear();

    // Initial dummy state
    STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSU] = D3DTADDRESS_MIRROR;
    STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSV] = D3DTADDRESS_MIRROR;
    STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSW] = D3DTADDRESS_MIRROR;

    {
        auto scope = SamplerAddressScope::Wrap(0);
        
        assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSU] == D3DTADDRESS_WRAP);
        assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSV] == D3DTADDRESS_WRAP);
        assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSW] == D3DTADDRESS_WRAP);
        
        assert(STATEMANAGER.getSamplerStateCalls == 3);
        assert(STATEMANAGER.setSamplerStateCalls == 3);
    } // scope goes out of scope and should restore

    assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSU] == D3DTADDRESS_MIRROR);
    assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSV] == D3DTADDRESS_MIRROR);
    assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSW] == D3DTADDRESS_MIRROR);
    assert(STATEMANAGER.setSamplerStateCalls == 6); // 3 for set, 3 for restore

    std::cout << "TestSamplerAddressScope_Wrap passed." << std::endl;
}

void TestSamplerAddressScope_Clamp()
{
    STATEMANAGER.ResetCounters();
    STATEMANAGER.samplerStates.clear();

    STATEMANAGER.samplerStates[1][D3DSAMP_ADDRESSU] = D3DTADDRESS_WRAP;
    STATEMANAGER.samplerStates[1][D3DSAMP_ADDRESSV] = D3DTADDRESS_WRAP;
    STATEMANAGER.samplerStates[1][D3DSAMP_ADDRESSW] = D3DTADDRESS_WRAP;

    {
        auto scope = SamplerAddressScope::Clamp(1);
        
        assert(STATEMANAGER.samplerStates[1][D3DSAMP_ADDRESSU] == D3DTADDRESS_CLAMP);
        assert(STATEMANAGER.samplerStates[1][D3DSAMP_ADDRESSV] == D3DTADDRESS_CLAMP);
        assert(STATEMANAGER.samplerStates[1][D3DSAMP_ADDRESSW] == D3DTADDRESS_CLAMP);
    }

    assert(STATEMANAGER.samplerStates[1][D3DSAMP_ADDRESSU] == D3DTADDRESS_WRAP);
    assert(STATEMANAGER.samplerStates[1][D3DSAMP_ADDRESSV] == D3DTADDRESS_WRAP);
    assert(STATEMANAGER.samplerStates[1][D3DSAMP_ADDRESSW] == D3DTADDRESS_WRAP);

    std::cout << "TestSamplerAddressScope_Clamp passed." << std::endl;
}

void TestSamplerAddressScope_Border()
{
    STATEMANAGER.ResetCounters();
    STATEMANAGER.samplerStates.clear();

    STATEMANAGER.samplerStates[2][D3DSAMP_ADDRESSU] = D3DTADDRESS_CLAMP;
    STATEMANAGER.samplerStates[2][D3DSAMP_ADDRESSV] = D3DTADDRESS_CLAMP;
    STATEMANAGER.samplerStates[2][D3DSAMP_ADDRESSW] = D3DTADDRESS_CLAMP;

    {
        auto scope = SamplerAddressScope::Border(2);
        
        assert(STATEMANAGER.samplerStates[2][D3DSAMP_ADDRESSU] == D3DTADDRESS_BORDER);
        assert(STATEMANAGER.samplerStates[2][D3DSAMP_ADDRESSV] == D3DTADDRESS_BORDER);
        assert(STATEMANAGER.samplerStates[2][D3DSAMP_ADDRESSW] == D3DTADDRESS_BORDER);
    }

    assert(STATEMANAGER.samplerStates[2][D3DSAMP_ADDRESSU] == D3DTADDRESS_CLAMP);
    assert(STATEMANAGER.samplerStates[2][D3DSAMP_ADDRESSV] == D3DTADDRESS_CLAMP);
    assert(STATEMANAGER.samplerStates[2][D3DSAMP_ADDRESSW] == D3DTADDRESS_CLAMP);

    std::cout << "TestSamplerAddressScope_Border passed." << std::endl;
}

void TestSamplerAddressScope_DistinctAxes()
{
    STATEMANAGER.ResetCounters();
    STATEMANAGER.samplerStates.clear();

    STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSU] = D3DTADDRESS_MIRROR;
    STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSV] = D3DTADDRESS_MIRROR;
    STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSW] = D3DTADDRESS_MIRROR;

    {
        SamplerAddressScope scope(0, D3DTADDRESS_WRAP, D3DTADDRESS_CLAMP, D3DTADDRESS_BORDER);
        
        assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSU] == D3DTADDRESS_WRAP);
        assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSV] == D3DTADDRESS_CLAMP);
        assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSW] == D3DTADDRESS_BORDER);
    }

    assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSU] == D3DTADDRESS_MIRROR);
    assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSV] == D3DTADDRESS_MIRROR);
    assert(STATEMANAGER.samplerStates[0][D3DSAMP_ADDRESSW] == D3DTADDRESS_MIRROR);

    std::cout << "TestSamplerAddressScope_DistinctAxes passed." << std::endl;
}

int main()
{
    TestSamplerAddressScope_Wrap();
    TestSamplerAddressScope_Clamp();
    TestSamplerAddressScope_Border();
    TestSamplerAddressScope_DistinctAxes();
    std::cout << "All C++23 SamplerAddressScope tests passed successfully!" << std::endl;
    return 0;
}
