#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "FastBootOverlay.h"

void CPythonNetworkStream::OnRemoteDisconnect()
{
	FastBootOverlay::Instance().OnRemoteDisconnect();
	if (m_poHandler)
	{
		PyCallClassMemberFunc(m_poHandler, "SetLoginPhase", Py_BuildValue("()"));
	}
}

void CPythonNetworkStream::OnDisconnect()
{
	FastBootOverlay::Instance().OnRemoteDisconnect();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main Game
void CPythonNetworkStream::OnScriptEventStart(int iSkin, int iIndex)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OpenQuestWindow", Py_BuildValue("(ii)", iSkin, iIndex));
}
