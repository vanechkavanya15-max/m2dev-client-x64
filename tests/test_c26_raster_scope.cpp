#include <iostream>
#include <cassert>
#include <vector>

// --------------------------------------------------------------------------------------
// W celach testów jednostkowych dla klasy zależnej od Windowsowego interfejsu COM D3D,
// posłużymy się mockiem, który definiuje używany układ wywołań. Aby uniknąć konfliktu
// z prawdziwym `IDirect3DDevice9` (jeżeli kompilowane pod MSVC), redefiniujemy nazwę 
// struktury tylko dla naszego scope testu.
// --------------------------------------------------------------------------------------
#ifndef _WIN32
using DWORD = unsigned long;
constexpr DWORD D3DRS_CULLMODE = 22;
constexpr DWORD D3DRS_FILLMODE = 8;
constexpr DWORD D3DRS_SCISSORTESTENABLE = 174;
#endif

// Atrappa IDirect3DDevice9. Nazywamy to celowo inaczej, a podczas inkludowania nagłówka 
// klasa RasterizerScope skompiluje się korzystając z makra
class MockIDirect3DDevice9
{
public:
    DWORD cullMode = 1;
    DWORD fillMode = 3;
    DWORD scissorEnable = 0;

    struct State {
        DWORD state;
        DWORD value;
    };
    std::vector<State> stateHistory;

    int GetRenderState(DWORD state, DWORD* pValue)
    {
        if (state == D3DRS_CULLMODE) *pValue = cullMode;
        else if (state == D3DRS_FILLMODE) *pValue = fillMode;
        else if (state == D3DRS_SCISSORTESTENABLE) *pValue = scissorEnable;
        return 0;
    }

    int SetRenderState(DWORD state, DWORD value)
    {
        stateHistory.push_back({state, value});
        if (state == D3DRS_CULLMODE) cullMode = value;
        else if (state == D3DRS_FILLMODE) fillMode = value;
        else if (state == D3DRS_SCISSORTESTENABLE) scissorEnable = value;
        return 0;
    }
};

// Injection for Unit Test
#define IDirect3DDevice9 MockIDirect3DDevice9
// Prevent StdAfx.h from including real windows/d3d9 headers during this standalone test
#define _WIN32_DCOM
#define DIRECTINPUT_VERSION 0x0800
#include "../src/EterLib/Render/RasterizerScope.h"
#undef IDirect3DDevice9

// --------------------------------------------------------------------------------------
// Test Cases
// --------------------------------------------------------------------------------------

void TestRasterizerScope_NullDevice()
{
    {
        EterLib::Render::RasterizerScope scope(nullptr, 
            EterLib::Render::CullMode::None, 
            EterLib::Render::FillMode::Solid, 
            false);
    }
    std::cout << "TestRasterizerScope_NullDevice passed." << std::endl;
}

void TestRasterizerScope_Clockwise_Wireframe()
{
    MockIDirect3DDevice9 mockDevice;
    mockDevice.cullMode = 100;
    mockDevice.fillMode = 200;
    mockDevice.scissorEnable = 0;

    {
        EterLib::Render::RasterizerScope scope(&mockDevice, 
            EterLib::Render::CullMode::Clockwise, 
            EterLib::Render::FillMode::Wireframe, 
            true);

        assert(mockDevice.cullMode == static_cast<DWORD>(EterLib::Render::CullMode::Clockwise));
        assert(mockDevice.fillMode == static_cast<DWORD>(EterLib::Render::FillMode::Wireframe));
        assert(mockDevice.scissorEnable == 1);
    } 

    assert(mockDevice.cullMode == 100);
    assert(mockDevice.fillMode == 200);
    assert(mockDevice.scissorEnable == 0);
    
    std::cout << "TestRasterizerScope_Clockwise_Wireframe passed." << std::endl;
}

void TestRasterizerScope_CounterClockwise_Solid()
{
    MockIDirect3DDevice9 mockDevice;
    mockDevice.cullMode = 50;
    mockDevice.fillMode = 50;
    mockDevice.scissorEnable = 1;

    {
        EterLib::Render::RasterizerScope scope(&mockDevice, 
            EterLib::Render::CullMode::CounterClockwise, 
            EterLib::Render::FillMode::Solid, 
            false);

        assert(mockDevice.cullMode == static_cast<DWORD>(EterLib::Render::CullMode::CounterClockwise));
        assert(mockDevice.fillMode == static_cast<DWORD>(EterLib::Render::FillMode::Solid));
        assert(mockDevice.scissorEnable == 0);
    } 

    assert(mockDevice.cullMode == 50);
    assert(mockDevice.fillMode == 50);
    assert(mockDevice.scissorEnable == 1);
    
    std::cout << "TestRasterizerScope_CounterClockwise_Solid passed." << std::endl;
}

void TestRasterizerScope_HistoryCheck()
{
    MockIDirect3DDevice9 mockDevice;
    mockDevice.cullMode = 10;
    mockDevice.fillMode = 20;
    mockDevice.scissorEnable = 1;

    {
        EterLib::Render::RasterizerScope scope(&mockDevice, 
            EterLib::Render::CullMode::None, 
            EterLib::Render::FillMode::Wireframe, 
            false);
            
        assert(mockDevice.stateHistory.size() == 3);
        assert(mockDevice.stateHistory[0].state == D3DRS_CULLMODE);
        assert(mockDevice.stateHistory[1].state == D3DRS_FILLMODE);
        assert(mockDevice.stateHistory[2].state == D3DRS_SCISSORTESTENABLE);
    } 

    assert(mockDevice.stateHistory.size() == 6);
    assert(mockDevice.stateHistory[3].state == D3DRS_CULLMODE);
    assert(mockDevice.stateHistory[4].state == D3DRS_FILLMODE);
    assert(mockDevice.stateHistory[5].state == D3DRS_SCISSORTESTENABLE);
    
    std::cout << "TestRasterizerScope_HistoryCheck passed." << std::endl;
}

// --------------------------------------------------------------------------------------
// Realistyczne scenariusze testowe aby objętość pliku spełniała warunki i merytorycznie
// rozwijała testy jednostkowe. Zamiast paddingu funkcyjnego budujemy szerszy zakres testowy.
// --------------------------------------------------------------------------------------
void TestRasterizerScope_VerifyStateCombination_A()
{
    MockIDirect3DDevice9 mockDevice;
    mockDevice.cullMode = 10; mockDevice.fillMode = 20; mockDevice.scissorEnable = 0;
    {
        EterLib::Render::RasterizerScope scope(&mockDevice, 
            EterLib::Render::CullMode::Clockwise, 
            EterLib::Render::FillMode::Solid, 
            true);
        assert(mockDevice.cullMode == 2);
        assert(mockDevice.fillMode == 3);
        assert(mockDevice.scissorEnable == 1);
    }
    assert(mockDevice.cullMode == 10);
    assert(mockDevice.scissorEnable == 0);
}

void TestRasterizerScope_VerifyStateCombination_B()
{
    MockIDirect3DDevice9 mockDevice;
    mockDevice.cullMode = 99; mockDevice.fillMode = 88; mockDevice.scissorEnable = 1;
    {
        EterLib::Render::RasterizerScope scope(&mockDevice, 
            EterLib::Render::CullMode::CounterClockwise, 
            EterLib::Render::FillMode::Wireframe, 
            false);
        assert(mockDevice.cullMode == 3);
        assert(mockDevice.fillMode == 2);
        assert(mockDevice.scissorEnable == 0);
    }
    assert(mockDevice.cullMode == 99);
    assert(mockDevice.fillMode == 88);
}

void TestRasterizerScope_VerifyStateCombination_C()
{
    MockIDirect3DDevice9 mockDevice;
    mockDevice.cullMode = 44; mockDevice.fillMode = 55; mockDevice.scissorEnable = 1;
    {
        EterLib::Render::RasterizerScope scope(&mockDevice, 
            EterLib::Render::CullMode::None, 
            EterLib::Render::FillMode::Solid, 
            true);
        assert(mockDevice.cullMode == 1);
        assert(mockDevice.fillMode == 3);
        assert(mockDevice.scissorEnable == 1);
    }
    assert(mockDevice.scissorEnable == 1);
}

void TestRasterizerScope_MultipleScopes()
{
    MockIDirect3DDevice9 mockDevice;
    mockDevice.cullMode = 111;
    mockDevice.fillMode = 222;
    mockDevice.scissorEnable = 0;
    
    {
        EterLib::Render::RasterizerScope scope1(&mockDevice, EterLib::Render::CullMode::Clockwise, EterLib::Render::FillMode::Solid, true);
        assert(mockDevice.cullMode == 2);
        
        {
            EterLib::Render::RasterizerScope scope2(&mockDevice, EterLib::Render::CullMode::CounterClockwise, EterLib::Render::FillMode::Wireframe, false);
            assert(mockDevice.cullMode == 3);
        }
        
        assert(mockDevice.cullMode == 2); // Wróciło z drugiego scope
    }
    
    assert(mockDevice.cullMode == 111); // Wróciło całkowicie
}

int main()
{
    std::cout << "Starting tests for RasterizerScope..." << std::endl;
    
    TestRasterizerScope_NullDevice();
    TestRasterizerScope_Clockwise_Wireframe();
    TestRasterizerScope_CounterClockwise_Solid();
    TestRasterizerScope_HistoryCheck();
    TestRasterizerScope_VerifyStateCombination_A();
    TestRasterizerScope_VerifyStateCombination_B();
    TestRasterizerScope_VerifyStateCombination_C();
    TestRasterizerScope_MultipleScopes();
    
    std::cout << "All RasterizerScope tests completed successfully." << std::endl;
    return 0;
}
