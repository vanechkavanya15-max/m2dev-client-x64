#include "../StdAfx.h"
#include "ICharacterAppearanceService.h"
#include "../PythonCharacterManager.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"

namespace UserInterface::Actors
{
    /**
     * @brief Implementacja modyfikacji fryzury w architekturze C++23.
     */
    class AppearanceService_Hair
    {
    public:
        /**
         * @brief Zmienia fryzure (hair) wybranej instancji gracza lub NPC.
         * @param id Identyfikator instancji (VID)
         * @param hairVnum Vnum nowej fryzury
         * @return EterBase::PacketResult<void> sukces lub PacketError w przypadku nieznalezienia instancji.
         */
        static EterBase::PacketResult<void> SetHair(EterBase::EntityId id, uint32_t hairVnum)
        {
            auto& charMgr = CPythonCharacterManager::Instance();
            CInstanceBase* instance = charMgr.GetInstancePtr(id.value());

            if (!instance)
            {
                EterBase::ModernLogger::Error("AppearanceService_Hair: Character instance {} not found", id.value());
                return std::unexpected(EterBase::PacketError::MalformedPayload); 
            }

            instance->ChangeHair(hairVnum);
            EterBase::ModernLogger::Info("AppearanceService_Hair: Hair updated to {} for instance {}", hairVnum, id.value());

            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(id.value()));

            return {};
        }
    };
}
