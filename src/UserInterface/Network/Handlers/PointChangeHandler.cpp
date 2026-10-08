#include "StdAfx.h"
#include "PointChangeHandler.h"
#include "../../Packet.h"
#include "../../PythonCharacterManager.h"
#include "../../PythonPlayer.h"
#include "../../InstanceBase.h"

bool PointChangeHandler::HandlePointChange(const TPacketGCPointChange& pointChange)
{
    CPythonCharacterManager& characterManager = CPythonCharacterManager::Instance();
    characterManager.ShowPointEffect(pointChange.Type, pointChange.dwVID);

    CInstanceBase* mainInstance = characterManager.GetMainActorPtr();
    
    // If the point change affects the main character
    if (mainInstance && pointChange.dwVID == mainInstance->GetVirtualID())
    {
        CPythonPlayer& player = CPythonPlayer::Instance();
        player.SetStatus(pointChange.Type, pointChange.value);

        if (pointChange.Type == POINT_ENERGY && pointChange.value == 0)
        {
            player.SetStatus(POINT_ENERGY_END_TIME, 0);
        }
    }
    else
    {
        // If the point change affects another character (e.g., leveling up)
        if (pointChange.Type == POINT_LEVEL)
        {
            CInstanceBase* targetInstance = characterManager.GetInstancePtr(pointChange.dwVID);
            if (targetInstance)
            {
                targetInstance->SetLevel(pointChange.value);
                targetInstance->UpdateTextTailLevel(pointChange.value);
            }
        }
    }

    return true;
}
