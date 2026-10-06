#include "StdAfx.h"
#include "UserInterface/Packet.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/PythonCharacterManager.h"
#include "UserInterface/InstanceBase.h"
#include "UserInterface/Core/EventBus.h"

#include <string>

namespace UserInterface::Core::Events {
    /**
     * @brief Zdarzenie wywolywane w momencie otrzymania zaproszenia do grupy.
     */
    struct PartyInviteReceivedEvent : public UserInterface::Core::IEvent {
        EterBase::EntityId leaderId; ///< Unikalny identyfikator lidera zapraszajacego do grupy.
        std::string leaderName;      ///< Nazwa lidera zapraszajacego do grupy.

        /**
         * @brief Konstruktor zdarzenia zaproszenia do grupy.
         * @param leaderId Unikalny identyfikator lidera.
         * @param leaderName Nazwa lidera.
         */
        PartyInviteReceivedEvent(EterBase::EntityId leaderId, std::string leaderName)
            : leaderId(leaderId), leaderName(std::move(leaderName)) {}
    };
} // namespace UserInterface::Core::Events

namespace UserInterface::Network::Handlers {

    /**
     * @brief Obsluguje przychodzacy pakiet zaproszenia do grupy od lidera.
     * 
     * Odbiera pakiet, weryfikuje istnienie instancji lidera w menedzerze postaci, 
     * publikuje zdarzenie (EventBus) celem powiadomienia innych podsystemow o zaproszeniu.
     * Nie posiada bezposrednich zaleznosci od interfejsu GUI (ZASADA ZERO-CONFLICT).
     * 
     * @param packet Referencja do struktury pakietu zaproszenia do grupy.
     * @return Zwraca sukces (std::expected<void, PacketError>) lub blad w przypadku braku nadawcy zaproszenia.
     */
    EterBase::PacketResult<void> HandlePartyInvitePacket(const TPacketGCPartyInvite& packet) {
        EterBase::EntityId leaderId{packet.leader_pid};

        CInstanceBase* instance = CPythonCharacterManager::Instance().GetInstancePtr(leaderId.get());
        if (!instance) {
            EterBase::ModernLogger::Error("PartyInviteHandler: Failed to find leader instance [{}]", leaderId.get());
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        std::string leaderName = instance->GetNameString();
        
        UserInterface::Core::EventBus::GetInstance().Publish(
            UserInterface::Core::Events::PartyInviteReceivedEvent{leaderId, leaderName}
        );

        EterBase::ModernLogger::Info("PartyInviteHandler: Successfully published PartyInviteReceivedEvent for leader [{}] ({})", leaderId.get(), leaderName);

        return {};
    }

} // namespace UserInterface::Network::Handlers
