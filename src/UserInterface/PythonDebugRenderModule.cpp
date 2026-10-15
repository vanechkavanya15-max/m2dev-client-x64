#include "StdAfx.h"
#include "PythonDebugRenderModule.h"
#include "PythonCharacterManager.h"
#include "PythonPlayer.h"
#include "InstanceBase.h"
#include "Client/Graphics/Debug/DebugRenderer.h"
#include "EterPythonLib/PythonGraphic.h"

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace
{
	bool s_debugRenderEnabled = true;

	bool ParseColor(PyObject* poArgs, int pos, uint32_t* pOutColor)
	{
		if (!pOutColor || pos >= PyTuple_Size(poArgs))
			return false;

		PyObject* poItem = PyTuple_GetItem(poArgs, pos);
		if (!poItem)
			return false;

		if (PyLong_Check(poItem))
		{
			*pOutColor = static_cast<uint32_t>(PyLong_AsUnsignedLongMask(poItem));
			return true;
		}
		if (PyInt_Check(poItem))
		{
			*pOutColor = static_cast<uint32_t>(PyInt_AsLong(poItem));
			return true;
		}
		return false;
	}

	bool ParseBool(PyObject* poArgs, int pos, bool* pOutBool)
	{
		if (!pOutBool || pos >= PyTuple_Size(poArgs))
			return false;

		PyObject* poItem = PyTuple_GetItem(poArgs, pos);
		if (!poItem)
			return false;

		if (PyBool_Check(poItem))
		{
			*pOutBool = (poItem == Py_True);
			return true;
		}
		if (PyLong_Check(poItem) || PyInt_Check(poItem))
		{
			*pOutBool = (PyLong_AsLong(poItem) != 0);
			return true;
		}
		return false;
	}
}

PyObject* debugRenderEnable(PyObject* poSelf, PyObject* poArgs)
{
	int enable = 1;
	if (!PyArg_ParseTuple(poArgs, "i", &enable))
		return Py_BadArgument();

	s_debugRenderEnabled = (enable != 0);
	return Py_BuildNone();
}

PyObject* debugRenderIsEnabled(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", s_debugRenderEnabled ? 1 : 0);
}

PyObject* debugRenderDrawLine(PyObject* poSelf, PyObject* poArgs)
{
	if (!s_debugRenderEnabled)
		return Py_BuildNone();

	const int argc = PyTuple_Size(poArgs);
	if (argc < 7 || argc > 8)
		return Py_BadArgument();

	float x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
	float x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;

	if (!PyTuple_GetFloat(poArgs, 0, &x1) ||
		!PyTuple_GetFloat(poArgs, 1, &y1) ||
		!PyTuple_GetFloat(poArgs, 2, &z1) ||
		!PyTuple_GetFloat(poArgs, 3, &x2) ||
		!PyTuple_GetFloat(poArgs, 4, &y2) ||
		!PyTuple_GetFloat(poArgs, 5, &z2))
	{
		return Py_BadArgument();
	}

	uint32_t color = 0xFFFFFFFF;
	if (!ParseColor(poArgs, 6, &color))
		return Py_BadArgument();

	bool xray = false;
	if (argc == 8 && !ParseBool(poArgs, 7, &xray))
		return Py_BadArgument();

	Client::Graphics::Debug::DebugRenderer::Instance().AddLine(
		x1, y1, z1, x2, y2, z2, color, xray
	);

	return Py_BuildNone();
}

PyObject* debugRenderDrawTracer(PyObject* poSelf, PyObject* poArgs)
{
	if (!s_debugRenderEnabled)
		return Py_BuildNone();

	const int argc = PyTuple_Size(poArgs);
	if (argc < 2 || argc > 3)
		return Py_BadArgument();

	unsigned int vid = 0;
	if (!PyTuple_GetUnsignedInteger(poArgs, 0, &vid))
	{
		int iVid = 0;
		if (!PyTuple_GetInteger(poArgs, 0, &iVid) || iVid <= 0)
			return Py_BadArgument();
		vid = static_cast<unsigned int>(iVid);
	}

	if (vid == 0)
		return Py_BadArgument();

	uint32_t color = 0xFFFFFFFF;
	if (!ParseColor(poArgs, 1, &color))
		return Py_BadArgument();

	bool xray = false;
	if (argc == 3 && !ParseBool(poArgs, 2, &xray))
		return Py_BadArgument();

	Client::Graphics::Debug::DebugRenderer::Instance().AddTracer(
		static_cast<uint32_t>(vid), color, xray
	);

	return Py_BuildNone();
}

PyObject* debugRenderDrawBox(PyObject* poSelf, PyObject* poArgs)
{
	if (!s_debugRenderEnabled)
		return Py_BuildNone();

	const int argc = PyTuple_Size(poArgs);
	if (argc < 2 || argc > 3)
		return Py_BadArgument();

	unsigned int vid = 0;
	if (!PyTuple_GetUnsignedInteger(poArgs, 0, &vid))
	{
		int iVid = 0;
		if (!PyTuple_GetInteger(poArgs, 0, &iVid) || iVid <= 0)
			return Py_BadArgument();
		vid = static_cast<unsigned int>(iVid);
	}

	if (vid == 0)
		return Py_BadArgument();

	uint32_t color = 0xFFFFFFFF;
	if (!ParseColor(poArgs, 1, &color))
		return Py_BadArgument();

	bool xray = false;
	if (argc == 3 && !ParseBool(poArgs, 2, &xray))
		return Py_BadArgument();

	Client::Graphics::Debug::DebugRenderer::Instance().AddActorBox(
		static_cast<uint32_t>(vid), color, xray
	);

	return Py_BuildNone();
}

PyObject* debugRenderDrawAABB(PyObject* poSelf, PyObject* poArgs)
{
	if (!s_debugRenderEnabled)
		return Py_BuildNone();

	const int argc = PyTuple_Size(poArgs);
	if (argc < 7 || argc > 8)
		return Py_BadArgument();

	float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
	float maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;

	if (!PyTuple_GetFloat(poArgs, 0, &minX) ||
		!PyTuple_GetFloat(poArgs, 1, &minY) ||
		!PyTuple_GetFloat(poArgs, 2, &minZ) ||
		!PyTuple_GetFloat(poArgs, 3, &maxX) ||
		!PyTuple_GetFloat(poArgs, 4, &maxY) ||
		!PyTuple_GetFloat(poArgs, 5, &maxZ))
	{
		return Py_BadArgument();
	}

	uint32_t color = 0xFFFFFFFF;
	if (!ParseColor(poArgs, 6, &color))
		return Py_BadArgument();

	bool xray = false;
	if (argc == 8 && !ParseBool(poArgs, 7, &xray))
		return Py_BadArgument();

	Client::Graphics::Debug::DebugRenderer::Instance().AddBox(
		minX, minY, minZ, maxX, maxY, maxZ, color, xray
	);

	return Py_BuildNone();
}

PyObject* debugRenderDrawCircle(PyObject* poSelf, PyObject* poArgs)
{
	if (!s_debugRenderEnabled)
		return Py_BuildNone();

	const int argc = PyTuple_Size(poArgs);
	if (argc < 5 || argc > 7)
		return Py_BadArgument();

	float x = 0.0f, y = 0.0f, z = 0.0f, radius = 0.0f;
	if (!PyTuple_GetFloat(poArgs, 0, &x) ||
		!PyTuple_GetFloat(poArgs, 1, &y) ||
		!PyTuple_GetFloat(poArgs, 2, &z) ||
		!PyTuple_GetFloat(poArgs, 3, &radius))
	{
		return Py_BadArgument();
	}

	if (radius <= 0.0f)
		return Py_BadArgument();

	uint32_t color = 0xFFFFFFFF;
	if (!ParseColor(poArgs, 4, &color))
		return Py_BadArgument();

	bool xray = false;
	if (argc >= 6 && !ParseBool(poArgs, 5, &xray))
		return Py_BadArgument();

	int segments = 24;
	if (argc == 7 && !PyTuple_GetInteger(poArgs, 6, &segments))
		return Py_BadArgument();

	Client::Graphics::Debug::DebugRenderer::Instance().AddCircle(
		x, y, z, radius, color, xray, static_cast<uint32_t>(segments > 0 ? segments : 24)
	);

	return Py_BuildNone();
}

PyObject* debugRenderDrawCircleActor(PyObject* poSelf, PyObject* poArgs)
{
	if (!s_debugRenderEnabled)
		return Py_BuildNone();

	const int argc = PyTuple_Size(poArgs);
	if (argc < 3 || argc > 5)
		return Py_BadArgument();

	unsigned int vid = 0;
	if (!PyTuple_GetUnsignedInteger(poArgs, 0, &vid))
	{
		int iVid = 0;
		if (!PyTuple_GetInteger(poArgs, 0, &iVid) || iVid <= 0)
			return Py_BadArgument();
		vid = static_cast<unsigned int>(iVid);
	}

	float radius = 100.0f;
	if (!PyTuple_GetFloat(poArgs, 1, &radius) || radius <= 0.0f)
		return Py_BadArgument();

	uint32_t color = 0xFFFFFFFF;
	if (!ParseColor(poArgs, 2, &color))
		return Py_BadArgument();

	bool xray = false;
	if (argc >= 4 && !ParseBool(poArgs, 3, &xray))
		return Py_BadArgument();

	int segments = 24;
	if (argc == 5 && !PyTuple_GetInteger(poArgs, 4, &segments))
		return Py_BadArgument();

	float x = 0.0f, y = 0.0f, z = 0.0f;
	if (Client::Graphics::Debug::DebugRenderer::Instance().GetActorPosition(vid, x, y, z))
	{
		Client::Graphics::Debug::DebugRenderer::Instance().AddCircle(
			x, y, z, radius, color, xray, static_cast<uint32_t>(segments > 0 ? segments : 24)
		);
	}

	return Py_BuildNone();
}

PyObject* debugRenderDrawCross(PyObject* poSelf, PyObject* poArgs)
{
	if (!s_debugRenderEnabled)
		return Py_BuildNone();

	const int argc = PyTuple_Size(poArgs);
	if (argc < 3 || argc > 6)
		return Py_BadArgument();

	float x = 0.0f, y = 0.0f, z = 0.0f;
	if (!PyTuple_GetFloat(poArgs, 0, &x) ||
		!PyTuple_GetFloat(poArgs, 1, &y) ||
		!PyTuple_GetFloat(poArgs, 2, &z))
	{
		return Py_BadArgument();
	}

	float size = 50.0f;
	if (argc >= 4 && !PyTuple_GetFloat(poArgs, 3, &size))
		return Py_BadArgument();

	uint32_t color = 0xFFFFFFFF;
	if (argc >= 5 && !ParseColor(poArgs, 4, &color))
		return Py_BadArgument();

	bool xray = false;
	if (argc == 6 && !ParseBool(poArgs, 5, &xray))
		return Py_BadArgument();

	auto& renderer = Client::Graphics::Debug::DebugRenderer::Instance();
	renderer.AddLine(x - size, y, z, x + size, y, z, color, xray);
	renderer.AddLine(x, y - size, z, x, y + size, z, color, xray);
	renderer.AddLine(x, y, z, x, y, z + size * 1.5f, color, xray);

	return Py_BuildNone();
}

PyObject* debugRenderWorldToScreen(PyObject* poSelf, PyObject* poArgs)
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	if (!PyArg_ParseTuple(poArgs, "fff", &x, &y, &z))
		return Py_BadArgument();

	float screenX = 0.0f, screenY = 0.0f, screenZ = 0.0f;
	CPythonGraphic::Instance().ProjectPosition(x, y, z, &screenX, &screenY, &screenZ);

	const bool isVisible = (screenZ >= 0.0f && screenZ <= 1.0f);
	return Py_BuildValue("(ffi)", screenX, screenY, isVisible ? 1 : 0);
}

PyObject* debugRenderGetActorScreenPos(PyObject* poSelf, PyObject* poArgs)
{
	unsigned int vid = 0;
	if (!PyArg_ParseTuple(poArgs, "I", &vid))
		return Py_BadArgument();

	float x = 0.0f, y = 0.0f, z = 0.0f;
	if (!Client::Graphics::Debug::DebugRenderer::Instance().GetActorPosition(vid, x, y, z))
		return Py_BuildValue("(ffi)", 0.0f, 0.0f, 0);

	float screenX = 0.0f, screenY = 0.0f, screenZ = 0.0f;
	CPythonGraphic::Instance().ProjectPosition(x, y, z, &screenX, &screenY, &screenZ);

	const bool isVisible = (screenZ >= 0.0f && screenZ <= 1.0f);
	return Py_BuildValue("(ffi)", screenX, screenY, isVisible ? 1 : 0);
}

PyObject* debugRenderGetActorsInRadius(PyObject* poSelf, PyObject* poArgs)
{
	const int argc = PyTuple_Size(poArgs);
	if (argc < 1 || argc > 3)
		return Py_BadArgument();

	float radius = 0.0f;
	if (!PyTuple_GetFloat(poArgs, 0, &radius))
		return Py_BadArgument();

	if (radius <= 0.0f)
		return Py_BadArgument();

	int typeFilter = -1;
	if (argc >= 2 && !PyTuple_GetInteger(poArgs, 1, &typeFilter))
		return Py_BadArgument();

	bool includeDead = false;
	if (argc == 3 && !ParseBool(poArgs, 2, &includeDead))
		return Py_BadArgument();

	CInstanceBase* pMainActor = CPythonCharacterManager::Instance().GetMainActorPtr();
	if (!pMainActor)
		return PyTuple_New(0);

	TPixelPosition mainPos;
	pMainActor->NEW_GetPixelPosition(&mainPos);

	std::vector<DWORD> matchedVids;
	matchedVids.reserve(32);

	const float radiusSq = radius * radius;

	CPythonCharacterManager::Instance().for_each_actor([&](CInstanceBase* pActor) {
		if (!pActor || pActor == pMainActor)
			return;

		if (!includeDead && pActor->IsDead())
			return;

		if (typeFilter != -1 && pActor->GetInstanceType() != typeFilter)
			return;

		TPixelPosition actorPos;
		pActor->NEW_GetPixelPosition(&actorPos);

		const float dx = actorPos.x - mainPos.x;
		const float dy = actorPos.y - mainPos.y;
		const float dz = actorPos.z - mainPos.z;
		const float distSq = dx * dx + dy * dy + dz * dz;

		if (distSq <= radiusSq)
		{
			matchedVids.push_back(pActor->GetVirtualID());
		}
	});

	PyObject* poTuple = PyTuple_New(static_cast<Py_ssize_t>(matchedVids.size()));
	if (!poTuple)
		return PyErr_NoMemory();

	for (size_t i = 0; i < matchedVids.size(); ++i)
	{
		PyTuple_SetItem(poTuple, static_cast<Py_ssize_t>(i), PyLong_FromUnsignedLong(matchedVids[i]));
	}

	return poTuple;
}

PyObject* debugRenderClear(PyObject* poSelf, PyObject* poArgs)
{
	Client::Graphics::Debug::DebugRenderer::Instance().Clear();
	return Py_BuildNone();
}

PyObject* debugRenderGetCounts(PyObject* poSelf, PyObject* poArgs)
{
	const auto& renderer = Client::Graphics::Debug::DebugRenderer::Instance();
	return Py_BuildValue("(iii)",
		static_cast<int>(renderer.GetLineCount()),
		static_cast<int>(renderer.GetBoxCount()),
		static_cast<int>(renderer.GetCircleCount()));
}

void initdebugRender()
{
	static PyMethodDef s_methods[] =
	{
		{ "Enable",				debugRenderEnable,				METH_VARARGS },
		{ "IsEnabled",			debugRenderIsEnabled,			METH_VARARGS },
		{ "DrawLine",			debugRenderDrawLine,			METH_VARARGS },
		{ "DrawTracer",			debugRenderDrawTracer,			METH_VARARGS },
		{ "DrawBox",			debugRenderDrawBox,				METH_VARARGS },
		{ "DrawAABB",			debugRenderDrawAABB,			METH_VARARGS },
		{ "DrawCircle",			debugRenderDrawCircle,			METH_VARARGS },
		{ "DrawCircleActor",	debugRenderDrawCircleActor,		METH_VARARGS },
		{ "DrawCross",			debugRenderDrawCross,			METH_VARARGS },
		{ "WorldToScreen",		debugRenderWorldToScreen,		METH_VARARGS },
		{ "GetActorScreenPos",	debugRenderGetActorScreenPos,	METH_VARARGS },
		{ "GetActorsInRadius",	debugRenderGetActorsInRadius,	METH_VARARGS },
		{ "Clear",				debugRenderClear,				METH_VARARGS },
		{ "GetCounts",			debugRenderGetCounts,			METH_VARARGS },

		{ NULL,					NULL,							NULL		 },
	};

	PyObject* poModule = Py_InitModule("debugRender", s_methods);

	// Expose actor type constants for convenience
	PyModule_AddIntConstant(poModule, "TYPE_ENEMY", CActorInstance::TYPE_ENEMY);
	PyModule_AddIntConstant(poModule, "TYPE_NPC", CActorInstance::TYPE_NPC);
	PyModule_AddIntConstant(poModule, "TYPE_STONE", CActorInstance::TYPE_STONE);
	PyModule_AddIntConstant(poModule, "TYPE_WARP", CActorInstance::TYPE_WARP);
	PyModule_AddIntConstant(poModule, "TYPE_DOOR", CActorInstance::TYPE_DOOR);
	PyModule_AddIntConstant(poModule, "TYPE_BUILDING", CActorInstance::TYPE_BUILDING);
	PyModule_AddIntConstant(poModule, "TYPE_PC", CActorInstance::TYPE_PC);
	PyModule_AddIntConstant(poModule, "TYPE_POLY", CActorInstance::TYPE_POLY);
	PyModule_AddIntConstant(poModule, "TYPE_HORSE", CActorInstance::TYPE_HORSE);
	PyModule_AddIntConstant(poModule, "TYPE_GOTO", CActorInstance::TYPE_GOTO);
	PyModule_AddIntConstant(poModule, "TYPE_OBJECT", CActorInstance::TYPE_OBJECT);

	// Stale kolorow pomocniczych ARGB
	PyModule_AddIntConstant(poModule, "COLOR_RED",      0xFFFF0000);
	PyModule_AddIntConstant(poModule, "COLOR_GREEN",    0xFF00FF00);
	PyModule_AddIntConstant(poModule, "COLOR_BLUE",     0xFF0000FF);
	PyModule_AddIntConstant(poModule, "COLOR_YELLOW",   0xFFFFFF00);
	PyModule_AddIntConstant(poModule, "COLOR_CYAN",     0xFF00FFFF);
	PyModule_AddIntConstant(poModule, "COLOR_MAGENTA",  0xFFFF00FF);
	PyModule_AddIntConstant(poModule, "COLOR_WHITE",    0xFFFFFFFF);
	PyModule_AddIntConstant(poModule, "COLOR_ORANGE",   0xFFFFA500);

	// Dependency Injection: Pozycje aktorow dla DebugRenderer
	Client::Graphics::Debug::DebugRenderer::Instance().SetPositionProviders(
		[](uint32_t vid, float& outX, float& outY, float& outZ) -> bool
		{
			CInstanceBase* pActor = CPythonCharacterManager::Instance().GetInstancePtr(vid);
			if (!pActor)
				return false;

			TPixelPosition kPos;
			pActor->NEW_GetPixelPosition(&kPos);
			outX = kPos.x;
			outY = -kPos.y; // Inwersja osi Y dla Direct3D 9
			outZ = kPos.z;
			return true;
		},
		[](float& outX, float& outY, float& outZ) -> bool
		{
			const DWORD mainVid = CPythonPlayer::Instance().GetMainCharacterIndex();
			if (mainVid == 0)
				return false;

			CInstanceBase* pMainActor = CPythonCharacterManager::Instance().GetInstancePtr(mainVid);
			if (!pMainActor)
				return false;

			TPixelPosition kPos;
			pMainActor->NEW_GetPixelPosition(&kPos);
			outX = kPos.x;
			outY = -kPos.y; // Inwersja osi Y dla Direct3D 9
			outZ = kPos.z;
			return true;
		}
	);
}
