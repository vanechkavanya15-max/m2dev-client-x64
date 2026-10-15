#include "StdAfx.h"
#include "PythonChat.h"
#include "PythonItem.h"
#include "GameLib/ItemManager.h"
#include "EterBase/PyBridge.h"

PyObject * chatSetChatColor(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iType = 0;
	int32_t r = 0;
	int32_t g = 0;
	int32_t b = 0;
	if (!PyBridge::ExtractArgs(poArgs, iType, r, g, b))
		return Py_BuildException();

	CPythonChat::Instance().SetChatColor(iType, r, g, b);
	return Py_BuildNone();
}

PyObject * chatClear(PyObject* poSelf, PyObject* poArgs)
{
	CPythonChat::Instance().Destroy();
	return Py_BuildNone();
}

PyObject * chatClose(PyObject* poSelf, PyObject* poArgs)
{
	CPythonChat::Instance().Close();
	return Py_BuildNone();
}

PyObject * chatCreateChatSet(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonChat::Instance().CreateChatSet(iID));
}

PyObject * chatUpdate(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID))
		return Py_BuildException();

	CPythonChat::Instance().Update(iID);
	return Py_BuildNone();
}

PyObject * chatRender(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID))
		return Py_BuildException();

	CPythonChat::Instance().Render(iID);
	return Py_BuildNone();
}

PyObject * chatSetBoardState(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	int32_t iState = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID, iState))
		return Py_BuildException();

	CPythonChat::Instance().SetBoardState(iID, iState);
	return Py_BuildNone();
}

PyObject * chatSetPosition(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	int32_t ix = 0;
	int32_t iy = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID, ix, iy))
		return Py_BuildException();

	CPythonChat::Instance().SetPosition(iID, ix, iy);
	return Py_BuildNone();
}

PyObject* chatSetAlign(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	int32_t iAlign = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID, iAlign))
		return Py_BuildException();

	CPythonChat::Instance().SetAlign(iID, iAlign);
	return Py_BuildNone();
}

PyObject* chatSetWidth(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	int32_t iWidth = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID, iWidth))
		return Py_BadArgument();

	CPythonChat::Instance().SetWidth(static_cast<DWORD>(iID), iWidth);
	return Py_BuildNone();
}

PyObject * chatSetHeight(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	int32_t iHeight = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID, iHeight))
		return Py_BuildException();

	CPythonChat::Instance().SetHeight(iID, iHeight);
	return Py_BuildNone();
}

PyObject * chatSetStep(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	int32_t iStep = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID, iStep))
		return Py_BuildException();

	CPythonChat::Instance().SetStep(iID, iStep);
	return Py_BuildNone();
}

PyObject * chatToggleChatMode(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	int32_t iType = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID, iType))
		return Py_BuildException();

	CPythonChat::Instance().ToggleChatMode(iID, iType);
	return Py_BuildNone();
}

PyObject * chatEnableChatMode(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	int32_t iType = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID, iType))
		return Py_BuildException();

	CPythonChat::Instance().EnableChatMode(iID, iType);
	return Py_BuildNone();
}

PyObject * chatDisableChatMode(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	int32_t iType = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID, iType))
		return Py_BuildException();

	CPythonChat::Instance().DisableChatMode(iID, iType);
	return Py_BuildNone();
}

PyObject * chatSetEndPos(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	float fPos = 0.0f;
	if (!PyBridge::ExtractArgs(poArgs, iID, fPos))
		return Py_BuildException();

	CPythonChat::Instance().SetEndPos(iID, fPos);
	return Py_BuildNone();
}

PyObject * chatGetLineCount(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonChat::Instance().GetLineCount(iID));
}

PyObject * chatGetVisibleLineCount(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonChat::Instance().GetVisibleLineCount(iID));
}

PyObject * chatGetLineStep(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonChat::Instance().GetLineStep(iID));
}

PyObject * chatAppendChat(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iType = 0;
	const char* szChat = nullptr;
	if (!PyBridge::ExtractArgs(poArgs, iType, szChat))
		return Py_BuildException();

	CPythonChat::Instance().AppendChat(iType, szChat);
	return Py_BuildNone();
}

PyObject * chatAppendChatWithDelay(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iType = 0;
	const char* szChat = nullptr;
	int32_t iDelay = 0;
	if (!PyBridge::ExtractArgs(poArgs, iType, szChat, iDelay))
		return Py_BuildException();

	CPythonChat::Instance().AppendChatWithDelay(iType, szChat, iDelay);
	return Py_BuildNone();
}

PyObject * chatArrangeShowingChat(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iID = 0;
	if (!PyBridge::ExtractArgs(poArgs, iID))
		return Py_BuildException();

	CPythonChat::Instance().ArrangeShowingChat(iID);
	return Py_BuildNone();
}

PyObject * chatIgnoreCharacter(PyObject* poSelf, PyObject* poArgs)
{
	const char* szName = nullptr;
	if (!PyBridge::ExtractArgs(poArgs, szName))
		return Py_BuildException();

	CPythonChat::Instance().IgnoreCharacter(szName);
	return Py_BuildNone();
}

PyObject * chatIsIgnoreCharacter(PyObject* poSelf, PyObject* poArgs)
{
	const char* szName = nullptr;
	if (!PyBridge::ExtractArgs(poArgs, szName))
		return Py_BuildException();

	CPythonChat::Instance().IsIgnoreCharacter(szName);
	return Py_BuildNone();
}

PyObject * chatCreateWhisper(PyObject* poSelf, PyObject* poArgs)
{
	const char* szName = nullptr;
	if (!PyBridge::ExtractArgs(poArgs, szName))
		return Py_BuildException();

	CPythonChat::Instance().CreateWhisper(szName);
	return Py_BuildNone();
}

PyObject * chatAppendWhisper(PyObject* poSelf, PyObject* poArgs)
{
	int32_t iType = 0;
	const char* szName = nullptr;
	const char* szChat = nullptr;
	if (!PyBridge::ExtractArgs(poArgs, iType, szName, szChat))
		return Py_BuildException();

	CPythonChat::Instance().AppendWhisper(iType, szName, szChat);
	return Py_BuildNone();
}

PyObject * chatRenderWhisper(PyObject* poSelf, PyObject* poArgs)
{
	const char* szName = nullptr;
	float fx = 0.0f;
	float fy = 0.0f;
	if (!PyBridge::ExtractArgs(poArgs, szName, fx, fy))
		return Py_BuildException();

	CWhisper * pWhisper = nullptr;
	if (CPythonChat::Instance().GetWhisper(szName, &pWhisper))
	{
		pWhisper->Render(fx, fy);
	}

	return Py_BuildNone();
}

PyObject * chatSetWhisperBoxSize(PyObject* poSelf, PyObject* poArgs)
{
	const char* szName = nullptr;
	float fWidth = 0.0f;
	float fHeight = 0.0f;
	if (!PyBridge::ExtractArgs(poArgs, szName, fWidth, fHeight))
		return Py_BuildException();

	CWhisper * pWhisper = nullptr;
	if (CPythonChat::Instance().GetWhisper(szName, &pWhisper))
	{
		pWhisper->SetBoxSize(fWidth, fHeight);
	}

	return Py_BuildNone();
}

PyObject * chatSetWhisperPosition(PyObject* poSelf, PyObject* poArgs)
{
	const char* szName = nullptr;
	float fPosition = 0.0f;
	if (!PyBridge::ExtractArgs(poArgs, szName, fPosition))
		return Py_BuildException();

	CWhisper * pWhisper = nullptr;
	if (CPythonChat::Instance().GetWhisper(szName, &pWhisper))
	{
		pWhisper->SetPosition(fPosition);
	}

	return Py_BuildNone();
}

PyObject * chatClearWhisper(PyObject* poSelf, PyObject* poArgs)
{
	const char* szName = nullptr;
	if (!PyBridge::ExtractArgs(poArgs, szName))
		return Py_BuildException();

	CPythonChat::Instance().ClearWhisper(szName);
	return Py_BuildNone();
}

PyObject * chatInitWhisper(PyObject* poSelf, PyObject* poArgs)
{
	PyObject * poInterface = nullptr;
	if (!PyBridge::ExtractArgs(poArgs, poInterface))
		return Py_BuildException();

	PyBridge::PyRef<PyObject> refInterface(poInterface, true);
	CPythonChat::Instance().InitWhisper(refInterface.Get());
	return Py_BuildNone();
}

PyObject * chatGetLinkFromHyperlink(PyObject * poSelf, PyObject * poArgs)
{
	const char * szHyperlink = nullptr;
	if (!PyBridge::ExtractArgs(poArgs, szHyperlink))
		return Py_BuildException();

	std::string stHyperlink(szHyperlink);
	std::vector<std::string> results;

	split_string(stHyperlink, ":", results, false);

	// item:vnum:flag:socket0:socket1:socket2
	if (0 == results[0].compare("item"))
	{
		if (results.size() < 6)
			return Py_BuildValue("s", "");

		CItemData * pItemData = NULL;

		if (CItemManager::Instance().GetItemDataPointer(htoi(results[1].c_str()), &pItemData))
		{
			char buf[1024] = { 0 };
			char itemlink[256];
			int len;
			bool isAttr = false;

			len = snprintf(itemlink, sizeof(itemlink), "item:%x:%x:%x:%x:%x", 
					htoi(results[1].c_str()),
					htoi(results[2].c_str()),
					htoi(results[3].c_str()),
					htoi(results[4].c_str()),
					htoi(results[5].c_str()));

			if (results.size() >= 8)
			{
				for (int i = 6; i < results.size(); i += 2)
				{
					len += snprintf(itemlink + len, sizeof(itemlink) - len, ":%x:%I64d", 
							htoi(results[i].c_str()),
							atoi(results[i+1].c_str()));
					isAttr = true;
				}
			}

			if (isAttr)
				//"item:번호:플래그:소켓0:소켓1:소켓2"
				snprintf(buf, sizeof(buf), "|cffffc700|H%s|h[%s]|h|r", itemlink, pItemData->GetName());
			else
				snprintf(buf, sizeof(buf), "|cfff1e6c0|H%s|h[%s]|h|r", itemlink, pItemData->GetName());

			return Py_BuildValue("s", buf);
		}
	}

	return Py_BuildValue("s", "");
}

void initChat()
{
	static PyMethodDef s_methods[] = 
	{
		{ "SetChatColor",			chatSetChatColor,			METH_VARARGS },
		{ "Clear",					chatClear,					METH_VARARGS },
		{ "Close",					chatClose,					METH_VARARGS },

		{ "CreateChatSet",			chatCreateChatSet,			METH_VARARGS },
		{ "Update",					chatUpdate,					METH_VARARGS },
		{ "Render",					chatRender,					METH_VARARGS },

		{ "SetBoardState",			chatSetBoardState,			METH_VARARGS },
		{ "SetPosition",			chatSetPosition,			METH_VARARGS },
		{ "SetAlign",				chatSetAlign,				METH_VARARGS },
		{ "SetWidth",				chatSetWidth,				METH_VARARGS },
		{ "SetHeight",				chatSetHeight,				METH_VARARGS },
		{ "SetStep",				chatSetStep,				METH_VARARGS },
		{ "ToggleChatMode",			chatToggleChatMode,			METH_VARARGS },
		{ "EnableChatMode",			chatEnableChatMode,			METH_VARARGS },
		{ "DisableChatMode",		chatDisableChatMode,		METH_VARARGS },
		{ "SetEndPos",				chatSetEndPos,				METH_VARARGS },

		{ "GetLineCount",			chatGetLineCount,			METH_VARARGS },
		{ "GetVisibleLineCount",	chatGetVisibleLineCount,	METH_VARARGS },
		{ "GetLineStep",			chatGetLineStep,			METH_VARARGS },

		// Chat
		{ "AppendChat",				chatAppendChat,				METH_VARARGS },
		{ "AppendChatWithDelay",	chatAppendChatWithDelay,	METH_VARARGS },
		{ "ArrangeShowingChat",		chatArrangeShowingChat,		METH_VARARGS },

		// Ignore
		{ "IgnoreCharacter",		chatIgnoreCharacter,		METH_VARARGS },
		{ "IsIgnoreCharacter",		chatIsIgnoreCharacter,		METH_VARARGS },

		// Whisper
		{ "CreateWhisper",			chatCreateWhisper,			METH_VARARGS },
		{ "AppendWhisper",			chatAppendWhisper,			METH_VARARGS },
		{ "RenderWhisper",			chatRenderWhisper,			METH_VARARGS },
		{ "SetWhisperBoxSize",		chatSetWhisperBoxSize,		METH_VARARGS },
		{ "SetWhisperPosition",		chatSetWhisperPosition,		METH_VARARGS },
		{ "ClearWhisper",			chatClearWhisper,			METH_VARARGS },
		{ "InitWhisper",			chatInitWhisper,			METH_VARARGS },

		// Link
		{ "GetLinkFromHyperlink",	chatGetLinkFromHyperlink,	METH_VARARGS },

		{ NULL,						NULL,						NULL },
	};

	PyBridge::PyRef<PyObject> poModule(Py_InitModule("chat", s_methods), true);
	if (!poModule)
		return;

	PyModule_AddIntConstant(poModule.Get(), "CHAT_TYPE_TALKING",		CHAT_TYPE_TALKING);
	PyModule_AddIntConstant(poModule.Get(), "CHAT_TYPE_INFO",			CHAT_TYPE_INFO);
	PyModule_AddIntConstant(poModule.Get(), "CHAT_TYPE_NOTICE",			CHAT_TYPE_NOTICE);
	PyModule_AddIntConstant(poModule.Get(), "CHAT_TYPE_PARTY",			CHAT_TYPE_PARTY);
	PyModule_AddIntConstant(poModule.Get(), "CHAT_TYPE_GUILD",			CHAT_TYPE_GUILD);
	PyModule_AddIntConstant(poModule.Get(), "CHAT_TYPE_COMMAND",		CHAT_TYPE_COMMAND);
	PyModule_AddIntConstant(poModule.Get(), "CHAT_TYPE_SHOUT",			CHAT_TYPE_SHOUT);
	PyModule_AddIntConstant(poModule.Get(), "CHAT_TYPE_WHISPER",		CHAT_TYPE_WHISPER);
	PyModule_AddIntConstant(poModule.Get(), "WHISPER_TYPE_CHAT",		CPythonChat::WHISPER_TYPE_CHAT);
	PyModule_AddIntConstant(poModule.Get(), "WHISPER_TYPE_SYSTEM",		CPythonChat::WHISPER_TYPE_SYSTEM);
	PyModule_AddIntConstant(poModule.Get(), "WHISPER_TYPE_GM",			CPythonChat::WHISPER_TYPE_GM);

	PyModule_AddIntConstant(poModule.Get(), "BOARD_STATE_VIEW",			CPythonChat::BOARD_STATE_VIEW);
	PyModule_AddIntConstant(poModule.Get(), "BOARD_STATE_EDIT",			CPythonChat::BOARD_STATE_EDIT);
	PyModule_AddIntConstant(poModule.Get(), "BOARD_STATE_LOG",			CPythonChat::BOARD_STATE_LOG);

	PyModule_AddIntConstant(poModule.Get(), "CHAT_SET_CHAT_WINDOW",		0);
	PyModule_AddIntConstant(poModule.Get(), "CHAT_SET_LOG_WINDOW",		1);
}
