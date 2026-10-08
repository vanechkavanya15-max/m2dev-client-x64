#pragma once

#include "Client/Core/INetworkPort.h"
#include "Client/Core/Result.h"
#include "EterBase/StrongTypes.h"
#include "Protocol/ProtocolTypes.h" // For TItemPos

namespace Client::Network {

    /**
     * @brief Koder komend domeny wierzchowcow.
     * Implementuje zasade Zero-Conflict i C++23.
     */
    class MountCommandEncoder {
    public:
        explicit MountCommandEncoder(Client::Core::INetworkPort& port);

        /**
         * @brief Koduje i wysyla pakiet dosiadania konia.
         */
        Client::Core::Result<void, Client::Core::PacketError> EncodeMount(TItemPos mountItem);

        /**
         * @brief Koduje i wysyla pakiet zsiadania z konia.
         */
        Client::Core::Result<void, Client::Core::PacketError> EncodeDismount(TItemPos mountItem);

        /**
         * @brief Koduje i wysyla pakiet odpalenia umiejetnosci konnej.
         */
        Client::Core::Result<void, Client::Core::PacketError> EncodeUseHorseSkill(EterBase::SkillId skillId, EterBase::EntityId targetVid = EterBase::EntityId{0});

        /**
         * @brief Koduje i wysyla pakiet karmienia wierzchowca.
         */
        Client::Core::Result<void, Client::Core::PacketError> EncodeFeedMount(TItemPos foodPos, TItemPos mountPos);

    private:
        Client::Core::INetworkPort& m_port;
    };

} // namespace Client::Network
