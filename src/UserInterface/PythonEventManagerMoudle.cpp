#include "StdAfx.h"
#include "PythonEventManager.h"
#include "PythonNetworkStream.h"

PyObject * eventRegisterEventSet(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyUnicode_Check(poArgs[0]))
		return Py_BadArgument();

	const char * szFileName = PyUnicode_AsUTF8(poArgs[0]);
	if (!szFileName)
		return Py_BadArgument();

	int iEventIndex = CPythonEventManager::Instance().RegisterEventSet(szFileName);
	return PyLong_FromLong(iEventIndex);
}

PyObject * eventRegisterEventSetFromString(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyUnicode_Check(poArgs[0]))
		return Py_BadArgument();

	const char * szEventString = PyUnicode_AsUTF8(poArgs[0]);
	if (!szEventString)
		return Py_BadArgument();

	int iEventIndex = CPythonEventManager::Instance().RegisterEventSetFromString(szEventString);
	return PyLong_FromLong(iEventIndex);
}

PyObject * eventClearEventSet(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	CPythonEventManager::Instance().ClearEventSeti(iIndex);
	Py_RETURN_NONE;
}

PyObject * eventSetRestrictedCount(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 2)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]) || !PyLong_Check(poArgs[1]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	int iCount = PyLong_AsLong(poArgs[1]);

	CPythonEventManager::Instance().SetRestrictedCount(iIndex, iCount);
	Py_RETURN_NONE;
}

PyObject * eventGetEventSetLocalYPosition(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	return PyLong_FromLong(CPythonEventManager::Instance().GetEventSetLocalYPosition(iIndex));
}

PyObject * eventAddEventSetLocalYPosition(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 2)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]) || !PyLong_Check(poArgs[1]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	int iPos = PyLong_AsLong(poArgs[1]);

	CPythonEventManager::Instance().AddEventSetLocalYPosition(iIndex, iPos);
	Py_RETURN_NONE;
}

PyObject * eventInsertText(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 2)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]) || !PyUnicode_Check(poArgs[1]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	const char * szText = PyUnicode_AsUTF8(poArgs[1]);
	if (!szText)
		return Py_BadArgument();

	CPythonEventManager::Instance().InsertText(iIndex, szText);
	Py_RETURN_NONE;
}

PyObject * eventInsertTextInline(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 3)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]) || !PyUnicode_Check(poArgs[1]) || !PyLong_Check(poArgs[2]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	const char * szText = PyUnicode_AsUTF8(poArgs[1]);
	if (!szText)
		return Py_BadArgument();
	int iXIndex = PyLong_AsLong(poArgs[2]);

	CPythonEventManager::Instance().InsertText(iIndex, szText, iXIndex);
	Py_RETURN_NONE;
}

PyObject * eventUpdateEventSet(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 3)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]) || !PyLong_Check(poArgs[1]) || !PyLong_Check(poArgs[2]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	int ix = PyLong_AsLong(poArgs[1]);
	int iy = PyLong_AsLong(poArgs[2]);

	CPythonEventManager::Instance().UpdateEventSet(iIndex, ix, -iy);
	Py_RETURN_NONE;
}

PyObject * eventRenderEventSet(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	CPythonEventManager::Instance().RenderEventSet(iIndex);
	Py_RETURN_NONE;
}

PyObject * eventSetEventSetWidth(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 2)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]) || !PyLong_Check(poArgs[1]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	int iWidth = PyLong_AsLong(poArgs[1]);

	CPythonEventManager::Instance().SetEventSetWidth(iIndex, iWidth);
	Py_RETURN_NONE;
}

PyObject * eventSkip(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	CPythonEventManager::Instance().Skip(iIndex);
	Py_RETURN_NONE;
}

PyObject * eventIsWait(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	return PyBool_FromLong(CPythonEventManager::Instance().IsWait(iIndex));
}

PyObject * eventEndEventProcess(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	CPythonEventManager::Instance().EndEventProcess(iIndex);
	Py_RETURN_NONE;
}

PyObject * eventSelectAnswer(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 2)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]) || !PyLong_Check(poArgs[1]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	int iAnswer = PyLong_AsLong(poArgs[1]);

	CPythonEventManager::Instance().SelectAnswer(iIndex, iAnswer);
	Py_RETURN_NONE;
}

PyObject * eventGetLineCount(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	return PyLong_FromLong(CPythonEventManager::Instance().GetLineCount(iIndex));
}

PyObject * eventSetVisibleStartLine(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 2)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]) || !PyLong_Check(poArgs[1]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	int iStartLine = PyLong_AsLong(poArgs[1]);

	CPythonEventManager::Instance().SetVisibleStartLine(iIndex, iStartLine);
	Py_RETURN_NONE;
}

PyObject * eventGetVisibleStartLine(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	return PyLong_FromLong(CPythonEventManager::Instance().GetVisibleStartLine(iIndex));
}

PyObject * eventSetEventHandler(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 2)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	PyObject * poEventHandler = poArgs[1];

	CPythonEventManager::Instance().SetEventHandler(iIndex, poEventHandler);
	Py_RETURN_NONE;
}

PyObject * eventSetInterfaceWindow(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	PyObject * pyHandle = poArgs[0];
	CPythonEventManager::Instance().SetInterfaceWindow(pyHandle);
	Py_RETURN_NONE;
}

PyObject * eventSetLeftTimeString(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyUnicode_Check(poArgs[0]))
		return Py_BadArgument();

	const char * szText = PyUnicode_AsUTF8(poArgs[0]);
	if (!szText)
		return Py_BadArgument();

	CPythonEventManager::Instance().SetLeftTimeString(szText);
	Py_RETURN_NONE;
}

PyObject * eventQuestButtonClick(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	if (nargs != 1)
		return Py_BadArgument();

	if (!PyLong_Check(poArgs[0]))
		return Py_BadArgument();

	int iIndex = PyLong_AsLong(poArgs[0]);
	CPythonNetworkStream::Instance().SendScriptButtonPacket(iIndex);
	Py_RETURN_NONE;
}

PyObject * eventDestroy(PyObject * poSelf, PyObject * const * poArgs, Py_ssize_t nargs)
{
	CPythonEventManager::Instance().Destroy();
	Py_RETURN_NONE;
}

void initEvent()
{
	static PyMethodDef s_methods[] =
	{
		{ "RegisterEventSet",			(PyCFunction)eventRegisterEventSet,				METH_FASTCALL },
		{ "RegisterEventSetFromString",	(PyCFunction)eventRegisterEventSetFromString,	METH_FASTCALL },
		{ "ClearEventSet",				(PyCFunction)eventClearEventSet,				METH_FASTCALL },

		{ "SetRestrictedCount",			(PyCFunction)eventSetRestrictedCount,			METH_FASTCALL },

		{ "GetEventSetLocalYPosition",	(PyCFunction)eventGetEventSetLocalYPosition,	METH_FASTCALL },
		{ "AddEventSetLocalYPosition",	(PyCFunction)eventAddEventSetLocalYPosition,	METH_FASTCALL },
		{ "InsertText",					(PyCFunction)eventInsertText,					METH_FASTCALL },
		{ "InsertTextInline",			(PyCFunction)eventInsertTextInline,				METH_FASTCALL },

		{ "UpdateEventSet",				(PyCFunction)eventUpdateEventSet,				METH_FASTCALL },
		{ "RenderEventSet",				(PyCFunction)eventRenderEventSet,				METH_FASTCALL },
		{ "SetEventSetWidth",			(PyCFunction)eventSetEventSetWidth,				METH_FASTCALL },

		{ "Skip",						(PyCFunction)eventSkip,							METH_FASTCALL },
		{ "IsWait",						(PyCFunction)eventIsWait,						METH_FASTCALL },
		{ "EndEventProcess",			(PyCFunction)eventEndEventProcess,				METH_FASTCALL },

		{ "SelectAnswer",				(PyCFunction)eventSelectAnswer,					METH_FASTCALL },
		{ "GetLineCount",				(PyCFunction)eventGetLineCount,					METH_FASTCALL },
		{ "SetVisibleStartLine",		(PyCFunction)eventSetVisibleStartLine,			METH_FASTCALL },
		{ "GetVisibleStartLine",		(PyCFunction)eventGetVisibleStartLine,			METH_FASTCALL },

		{ "SetEventHandler",			(PyCFunction)eventSetEventHandler,				METH_FASTCALL },
		{ "SetInterfaceWindow",			(PyCFunction)eventSetInterfaceWindow,			METH_FASTCALL },
		{ "SetLeftTimeString",			(PyCFunction)eventSetLeftTimeString,			METH_FASTCALL },

		{ "QuestButtonClick",			(PyCFunction)eventQuestButtonClick,				METH_FASTCALL },
		{ "Destroy",					(PyCFunction)eventDestroy,						METH_FASTCALL },
		{ NULL,							NULL,											NULL          },
	};

	PyObject * poModule = Py_InitModule("event", s_methods);

	PyModule_AddIntConstant(poModule, "BOX_VISIBLE_LINE_COUNT", CPythonEventManager::BOX_VISIBLE_LINE_COUNT);
	PyModule_AddIntConstant(poModule, "BUTTON_TYPE_NEXT", CPythonEventManager::BUTTON_TYPE_NEXT);
	PyModule_AddIntConstant(poModule, "BUTTON_TYPE_DONE", CPythonEventManager::BUTTON_TYPE_DONE);
	PyModule_AddIntConstant(poModule, "BUTTON_TYPE_CANCEL", CPythonEventManager::BUTTON_TYPE_CANCEL);
}
