#ifndef __ETERLIB_RENDER_DYNAMICLIGHTSSCOPE_H__
#define __ETERLIB_RENDER_DYNAMICLIGHTSSCOPE_H__

#include <cstdint>
#include "../StateManager.h"

namespace EterLib::Render
{
	class DynamicLightsScope
	{
	public:
		DynamicLightsScope(DWORD index, bool bLightEnable, const D3DLIGHT9* pLight = nullptr)
			: m_index(index)
		{
			// Save current lighting global state
			STATEMANAGER.GetRenderState(D3DRS_LIGHTING, &m_oldLightEnable);

			// Save old light state
			STATEMANAGER.GetLight(index, &m_oldLight);

			// Apply new global light state
			STATEMANAGER.SetRenderState(D3DRS_LIGHTING, bLightEnable ? TRUE : FALSE);

			// Apply new light data if provided
			if (pLight)
			{
				STATEMANAGER.SetLight(index, pLight);
			}
		}

		~DynamicLightsScope()
		{
			// Restore old light state
			STATEMANAGER.SetLight(m_index, &m_oldLight);

			// Restore global lighting state
			STATEMANAGER.SetRenderState(D3DRS_LIGHTING, m_oldLightEnable);
		}

		// Delete copy and move
		DynamicLightsScope(const DynamicLightsScope&) = delete;
		DynamicLightsScope& operator=(const DynamicLightsScope&) = delete;
		DynamicLightsScope(DynamicLightsScope&&) = delete;
		DynamicLightsScope& operator=(DynamicLightsScope&&) = delete;

	private:
		DWORD m_index;
		DWORD m_oldLightEnable;
		D3DLIGHT9 m_oldLight;
	};
} // namespace EterLib::Render

#endif // __ETERLIB_RENDER_DYNAMICLIGHTSSCOPE_H__
