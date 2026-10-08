#include "../StdAfx.h"
#pragma once

#include "../StateManager.h"

namespace EterLib::Render
{
	class ViewProjectionScope
	{
	public:
		ViewProjectionScope(const D3DXMATRIX* pViewMatrix, const D3DXMATRIX* pProjMatrix)
		{
			STATEMANAGER.SaveTransform(D3DTS_VIEW, pViewMatrix);
			STATEMANAGER.SaveTransform(D3DTS_PROJECTION, pProjMatrix);
		}

		~ViewProjectionScope()
		{
			STATEMANAGER.RestoreTransform(D3DTS_PROJECTION);
			STATEMANAGER.RestoreTransform(D3DTS_VIEW);
		}

		// Delete copy and move semantics to prevent accidental multiple restorations
		ViewProjectionScope(const ViewProjectionScope&) = delete;
		ViewProjectionScope& operator=(const ViewProjectionScope&) = delete;
		ViewProjectionScope(ViewProjectionScope&&) = delete;
		ViewProjectionScope& operator=(ViewProjectionScope&&) = delete;
	};
}
