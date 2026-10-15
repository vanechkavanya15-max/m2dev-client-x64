#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <cstdint>

// Mock types to simulate D3D9 environment
typedef uint32_t DWORD;
typedef int32_t BOOL;
typedef int32_t INT;
typedef uint32_t UINT;
typedef int32_t HRESULT;
typedef void* LPDIRECT3DDEVICE9EX;
typedef void* LPDIRECT3DVERTEXBUFFER9;
typedef void* LPDIRECT3DINDEXBUFFER9;
typedef void* LPDIRECT3DBASETEXTURE9;
typedef void* LPDIRECT3DPIXELSHADER9;
typedef void* LPDIRECT3DVERTEXSHADER9;
typedef void* LPDIRECT3DVERTEXDECLARATION9;

struct D3DMATERIAL9 {};
struct D3DLIGHT9 {};
struct RECT { long left; long top; long right; long bottom; };
struct D3DXMATRIX { float m[4][4]; };

inline void D3DXMatrixIdentity(D3DXMATRIX* pOut) {}

enum D3DRENDERSTATETYPE { D3DRS_FOGENABLE, D3DRS_FOGCOLOR, D3DRS_FOGSTART, D3DRS_FOGEND, D3DRS_FOGDENSITY };
enum D3DTEXTURESTAGESTATETYPE { D3DTSS_COLOROP };
enum D3DSAMPLERSTATETYPE { D3DSAMP_MAGFILTER };
enum D3DTRANSFORMSTATETYPE { D3DTS_VIEW, D3DTS_PROJECTION, D3DTS_WORLD };
enum D3DPRIMITIVETYPE { D3DPT_TRIANGLELIST };
enum D3DFORMAT { D3DFMT_UNKNOWN };

#define S_OK 0
#define D3DFVF_XYZ 0x002
#define FALSE 0
#define TRUE 1
#define CONST const

// Mocking STATEMANAGER
class CStateManagerMock
{
public:
    static CStateManagerMock& Instance()
    {
        static CStateManagerMock instance;
        return instance;
    }

    void SaveRenderState(D3DRENDERSTATETYPE Type, DWORD dwValue)
    {
        m_saveHistory.push_back({Type, dwValue});
        m_renderStates[Type] = dwValue;
    }

    void RestoreRenderState(D3DRENDERSTATETYPE Type)
    {
        m_restoreHistory.push_back(Type);
    }

    struct SaveCall { D3DRENDERSTATETYPE type; DWORD dwValue; };
    std::vector<SaveCall> m_saveHistory;
    std::vector<D3DRENDERSTATETYPE> m_restoreHistory;
    std::map<D3DRENDERSTATETYPE, DWORD> m_renderStates;

    void ResetMock()
    {
        m_saveHistory.clear();
        m_restoreHistory.clear();
        m_renderStates.clear();
    }
};

#define STATEMANAGER (CStateManagerMock::Instance())

namespace EterLib::Render
{
	class FogDisableScope
	{
	public:
		FogDisableScope()
			: m_isFogEnabledSaved(true)
		{
			// Save current fog state and disable it
			STATEMANAGER.SaveRenderState(D3DRS_FOGENABLE, FALSE);
		}

		~FogDisableScope()
		{
			if (m_isFogEnabledSaved)
			{
				STATEMANAGER.RestoreRenderState(D3DRS_FOGENABLE);
			}
		}

		FogDisableScope(const FogDisableScope&) = delete;
		FogDisableScope& operator=(const FogDisableScope&) = delete;
		FogDisableScope(FogDisableScope&&) = delete;
		FogDisableScope& operator=(FogDisableScope&&) = delete;

	private:
		bool m_isFogEnabledSaved;
	};

	class FogStateScope
	{
	public:
		FogStateScope(bool enable, DWORD color, float start, float end, float density)
		{
			STATEMANAGER.SaveRenderState(D3DRS_FOGENABLE, enable ? TRUE : FALSE);
			STATEMANAGER.SaveRenderState(D3DRS_FOGCOLOR, color);
			STATEMANAGER.SaveRenderState(D3DRS_FOGSTART, *reinterpret_cast<DWORD*>(&start));
			STATEMANAGER.SaveRenderState(D3DRS_FOGEND, *reinterpret_cast<DWORD*>(&end));
			STATEMANAGER.SaveRenderState(D3DRS_FOGDENSITY, *reinterpret_cast<DWORD*>(&density));
		}

		~FogStateScope()
		{
			STATEMANAGER.RestoreRenderState(D3DRS_FOGENABLE);
			STATEMANAGER.RestoreRenderState(D3DRS_FOGCOLOR);
			STATEMANAGER.RestoreRenderState(D3DRS_FOGSTART);
			STATEMANAGER.RestoreRenderState(D3DRS_FOGEND);
			STATEMANAGER.RestoreRenderState(D3DRS_FOGDENSITY);
		}

		FogStateScope(const FogStateScope&) = delete;
		FogStateScope& operator=(const FogStateScope&) = delete;
		FogStateScope(FogStateScope&&) = delete;
		FogStateScope& operator=(FogStateScope&&) = delete;
	};
}

void TestFogDisableScope()
{
    STATEMANAGER.ResetMock();
    
    {
        EterLib::Render::FogDisableScope fogScope;
        assert(STATEMANAGER.m_saveHistory.size() == 1);
        assert(STATEMANAGER.m_saveHistory[0].type == D3DRS_FOGENABLE);
        assert(STATEMANAGER.m_saveHistory[0].dwValue == FALSE);
        assert(STATEMANAGER.m_restoreHistory.size() == 0);
    }
    
    assert(STATEMANAGER.m_restoreHistory.size() == 1);
    assert(STATEMANAGER.m_restoreHistory[0] == D3DRS_FOGENABLE);
    
    std::cout << "TestFogDisableScope passed!" << std::endl;
}

void TestFogStateScope()
{
    STATEMANAGER.ResetMock();
    
    float testStart = 100.0f;
    float testEnd = 1000.0f;
    float testDensity = 0.5f;
    DWORD testColor = 0xFFFFFFFF;
    
    {
        EterLib::Render::FogStateScope configScope(true, testColor, testStart, testEnd, testDensity);
        assert(STATEMANAGER.m_saveHistory.size() == 5);
        assert(STATEMANAGER.m_saveHistory[0].type == D3DRS_FOGENABLE);
        assert(STATEMANAGER.m_saveHistory[0].dwValue == TRUE);
        assert(STATEMANAGER.m_saveHistory[1].type == D3DRS_FOGCOLOR);
        assert(STATEMANAGER.m_saveHistory[1].dwValue == testColor);
        assert(STATEMANAGER.m_saveHistory[2].type == D3DRS_FOGSTART);
        assert(STATEMANAGER.m_saveHistory[2].dwValue == *reinterpret_cast<DWORD*>(&testStart));
        assert(STATEMANAGER.m_saveHistory[3].type == D3DRS_FOGEND);
        assert(STATEMANAGER.m_saveHistory[3].dwValue == *reinterpret_cast<DWORD*>(&testEnd));
        assert(STATEMANAGER.m_saveHistory[4].type == D3DRS_FOGDENSITY);
        assert(STATEMANAGER.m_saveHistory[4].dwValue == *reinterpret_cast<DWORD*>(&testDensity));
        assert(STATEMANAGER.m_restoreHistory.size() == 0);
    }
    
    assert(STATEMANAGER.m_restoreHistory.size() == 5);
    assert(STATEMANAGER.m_restoreHistory[0] == D3DRS_FOGENABLE);
    assert(STATEMANAGER.m_restoreHistory[1] == D3DRS_FOGCOLOR);
    assert(STATEMANAGER.m_restoreHistory[2] == D3DRS_FOGSTART);
    assert(STATEMANAGER.m_restoreHistory[3] == D3DRS_FOGEND);
    assert(STATEMANAGER.m_restoreHistory[4] == D3DRS_FOGDENSITY);
    
    std::cout << "TestFogStateScope passed!" << std::endl;
}

int main()
{
    TestFogDisableScope();
    TestFogStateScope();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
