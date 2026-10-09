#include "StdAfx.h"
#include "PythonPlayer.h"
#include "PythonPlayerModule.h"
#include "PythonApplication.h"
#include "PythonCharacterManager.h"

static std::map<int, CGraphicImage *> m_kMap_iEmotionIndex_pkIconImage;

PyObject * playerGetMainCharacterPosition(PyObject* poSelf, PyObject* poArgs)
{
	TPixelPosition kPPosMainActor;
	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	rkPlayer.NEW_GetMainActorPosition(&kPPosMainActor);
	return Py_BuildValue("fff", kPPosMainActor.x, kPPosMainActor.y, kPPosMainActor.z);
}

PyObject * playerGetCharacterDistance(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	CInstanceBase * pTargetInstance = CPythonCharacterManager::Instance().GetInstancePtr(iVID);
	if (!pMainInstance)
		return Py_BuildValue("f", -1.0f);
	if (!pTargetInstance)
		return Py_BuildValue("f", -1.0f);

	return Py_BuildValue("f", pMainInstance->GetDistance(pTargetInstance));
}

PyObject * playerSetQuickCameraMode(PyObject* poSelf, PyObject* poArgs)
{
	int nIsEnable;
	if (!PyTuple_GetInteger(poArgs, 0, &nIsEnable))
		return Py_BadArgument();

	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SetQuickCameraMode(nIsEnable ? true : false);	

	return Py_BuildNone();
}

// Test Code
PyObject* playerSetAutoCameraRotationSpeed(PyObject* poSelf, PyObject* poArgs)
{
	float fCmrRotSpd;
	if (!PyTuple_GetFloat(poArgs, 0, &fCmrRotSpd))
		return Py_BuildException();	

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.NEW_SetAutoCameraRotationSpeed(fCmrRotSpd);
	return Py_BuildNone();
}

PyObject * playerResetCameraRotation(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().NEW_ResetCameraRotation();
	return Py_BuildNone();
}

PyObject * playerSetSingleDIKKeyState(PyObject* poSelf, PyObject* poArgs)
{
	int eDIK;
	if (!PyTuple_GetInteger(poArgs, 0, &eDIK))
		return Py_BuildException("playerSetSingleDIKKeyState(eDIK, isPressed) - There is no first argument");

	int isPressed;
	if (!PyTuple_GetInteger(poArgs, 1, &isPressed))
		return Py_BuildException("playerSetSingleDIKKeyState(eDIK, isPressed) - There is no second argument");

	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	rkPlayer.NEW_SetSingleDIKKeyState(eDIK, isPressed ? true : false);
	return Py_BuildNone();
}

PyObject * playerEndKeyWalkingImmediately(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().NEW_Stop();
	return Py_BuildNone();
}


PyObject * playerStartMouseWalking(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildNone();
}

PyObject * playerEndMouseWalking(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildNone();
}

PyObject* playerSetMouseState(PyObject* poSelf, PyObject* poArgs)
{
	int eMBT;
	if (!PyTuple_GetInteger(poArgs, 0, &eMBT))
		return Py_BuildException();	

	int eMBS;
	if (!PyTuple_GetInteger(poArgs, 1, &eMBS))
		return Py_BuildException();	

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.NEW_SetMouseState(eMBT, eMBS);

	return Py_BuildNone();
}

PyObject* playerSetMouseFunc(PyObject* poSelf, PyObject* poArgs)
{
	int eMBT;
	if (!PyTuple_GetInteger(poArgs, 0, &eMBT))
		return Py_BuildException();	

	int eMBS;
	if (!PyTuple_GetInteger(poArgs, 1, &eMBS))
		return Py_BuildException();	

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.NEW_SetMouseFunc(eMBT, eMBS);

	return Py_BuildNone();
}

PyObject* playerGetMouseFunc(PyObject* poSelf, PyObject* poArgs)
{
	int eMBT;
	if (!PyTuple_GetInteger(poArgs, 0, &eMBT))
		return Py_BuildException();	

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	return Py_BuildValue("i", rkPlayer.NEW_GetMouseFunc(eMBT));
}

PyObject* playerSetMouseMiddleButtonState(PyObject* poSelf, PyObject* poArgs)
{
	int eMBS;
	if (!PyTuple_GetInteger(poArgs, 0, &eMBS))
		return Py_BuildException();	

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.NEW_SetMouseMiddleButtonState(eMBS);

	return Py_BuildNone();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PyObject * playerSetGameWindow(PyObject* poSelf, PyObject* poArgs)
{
	PyObject * pyHandle;
	if (!PyTuple_GetObject(poArgs, 0, &pyHandle))
		return Py_BadArgument();

	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SetGameWindow(pyHandle);
	return Py_BuildNone();
}


PyObject * playerShowPlayer(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (pMainInstance)
		pMainInstance->GetGraphicThingInstanceRef().Show();
	return Py_BuildNone();
}

PyObject * playerHidePlayer(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (pMainInstance)
		pMainInstance->GetGraphicThingInstanceRef().Hide();
	return Py_BuildNone();
}


PyObject * playerIsObserverMode(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	return Py_BuildValue("i", rkPlayer.IsObserverMode());
}

PyObject * playerIsMountingHorse(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (!pMainInstance)
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("i", pMainInstance->IsMountingHorse());
}

// Test Code
PyObject * playerSetMainCharacterIndex(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CPythonPlayer::Instance().SetMainCharacterIndex(iVID);
	CPythonCharacterManager::Instance().SetMainInstance(iVID);

	return Py_BuildNone();
}
// Test Code

PyObject * playerGetMainCharacterIndex(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetMainCharacterIndex());
}

PyObject * playerGetMainCharacterName(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("s", CPythonPlayer::Instance().GetName());
}

PyObject * playerIsMainCharacterIndex(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().IsMainCharacterIndex(iVID));
}

PyObject * playerGetName(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("s", CPythonPlayer::Instance().GetName()); 
}

PyObject * playerGetRace(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetRace());
}

PyObject * playerGetJob(PyObject* poSelf, PyObject* poArgs)
{
	int race = CPythonPlayer::Instance().GetRace();
	int job = RaceToJob(race);
	return Py_BuildValue("i", job);
}

PyObject * playerGetPlayTime(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetPlayTime()); 
}

PyObject * playerSetPlayTime(PyObject* poSelf, PyObject* poArgs)
{
	int iTime;
	if (!PyTuple_GetInteger(poArgs, 0, &iTime))
		return Py_BuildException();

	CPythonPlayer::Instance().SetPlayTime(iTime);
	return Py_BuildNone();
}

PyObject * playerIsActingEmotion(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (!pMainInstance)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pMainInstance->IsActingEmotion());
}

PyObject * playerActEmotion(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.ActEmotion(iVID);
	return Py_BuildNone();
}

PyObject * playerRegisterEmotionIcon(PyObject* poSelf, PyObject* poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	CGraphicImage * pImage = (CGraphicImage *)CResourceManager::Instance().GetResourcePointer(szFileName);
	m_kMap_iEmotionIndex_pkIconImage.insert(std::make_pair(iIndex, pImage));

	return Py_BuildNone();
}

PyObject * playerGetEmotionIconImage(PyObject* poSelf, PyObject* poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BuildException();

	if (m_kMap_iEmotionIndex_pkIconImage.end() == m_kMap_iEmotionIndex_pkIconImage.find(iIndex))
		return Py_BuildValue("K", 0);

	return Py_BuildValue("K", m_kMap_iEmotionIndex_pkIconImage[iIndex]);
}

PyObject * playerRegisterEffect(PyObject* poSelf, PyObject* poArgs)
{
	int iEft;
	if (!PyTuple_GetInteger(poArgs, 0, &iEft))
		return Py_BadArgument();

	char* szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BadArgument();

	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	if (!rkPlayer.RegisterEffect(iEft, szFileName, false))
		return Py_BuildException("CPythonPlayer::RegisterEffect(eEft=%d, szFileName=%s", iEft, szFileName);

	return Py_BuildNone();
}

PyObject * playerRegisterCacheEffect(PyObject* poSelf, PyObject* poArgs)
{
	int iEft;
	if (!PyTuple_GetInteger(poArgs, 0, &iEft))
		return Py_BadArgument();

	char* szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BadArgument();

	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	if (!rkPlayer.RegisterEffect(iEft, szFileName, true))
		return Py_BuildException("CPythonPlayer::RegisterEffect(eEft=%d, szFileName=%s", iEft, szFileName);

	return Py_BuildNone();
}

