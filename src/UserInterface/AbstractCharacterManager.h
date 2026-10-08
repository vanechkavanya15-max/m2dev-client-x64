#pragma once

#include "AbstractSingleton.h"

class CInstanceBase;

class IAbstractCharacterManager : public TAbstractSingleton<IAbstractCharacterManager>
{
	public:
		IAbstractCharacterManager() {}
		virtual ~IAbstractCharacterManager() {}

		virtual void Destroy() = 0;
		virtual CInstanceBase *						GetInstancePtr(DWORD dwVID) = 0;
		virtual CInstanceBase *						GetMainActorPtr() { return nullptr; }
		virtual CInstanceBase *						GetPickedActorPtr() { return nullptr; }
		virtual bool								GetPickedActorID(DWORD* pdwPickedActorID) { return false; }
		virtual void								RefreshAllPCTextTail() {}
		virtual void								ShowPointEffect(DWORD ePoint, DWORD dwVID) {}
		virtual void								ChangeGVG(DWORD dwSrcGuildID, DWORD dwDstGuildID) {}
};
