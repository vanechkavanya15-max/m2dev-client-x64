#pragma once

#include "../StdAfx.h"
#include "../StateManager.h"

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

		// Disable copying and moving
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

		// Disable copying and moving
		FogStateScope(const FogStateScope&) = delete;
		FogStateScope& operator=(const FogStateScope&) = delete;
		FogStateScope(FogStateScope&&) = delete;
		FogStateScope& operator=(FogStateScope&&) = delete;
	};
}
