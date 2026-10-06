#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"

#include <cstdint>
#include <span>
#include <cstring>
#include <array>

namespace UserInterface::Network
{
    /**
     * @brief Zdarzenie aktualizacji danych postaci wysylane na EventBus.
     */
    struct CharacterUpdateEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        std::array<uint16_t, CHR_EQUIPPART_NUM> parts;
        uint8_t movingSpeed;
        uint8_t attackSpeed;
        uint8_t stateFlag;
        std::array<uint32_t, 2> affectFlag;
        EterBase::GuildId guildId;
        int16_t alignment;
        uint8_t pkMode;
        EterBase::ItemVnum mountVnum;
        
        explicit CharacterUpdateEvent(const TPacketGCCharacterUpdate& packet)
            : entityId(EterBase::EntityId(packet.dwVID)),
              movingSpeed(packet.bMovingSpeed),
              attackSpeed(packet.bAttackSpeed),
              stateFlag(packet.bStateFlag),
              guildId(EterBase::GuildId(packet.dwGuildID)),
              alignment(packet.sAlignment),
              pkMode(packet.bPKMode),
              mountVnum(EterBase::ItemVnum(packet.dwMountVnum))
        {
            std::memcpy(parts.data(), packet.awPart, sizeof(packet.awPart));
            affectFlag[0] = packet.dwAffectFlag[0];
            affectFlag[1] = packet.dwAffectFlag[1];
        }
    };

    /**
     * @brief Handler aktualizacji danych postaci na podstawie TPacketGCCharacterUpdate.
     */
    class ActorPacket_CharUpdate
    {
    public:
        /**
         * @brief Przetwarza pakiet TPacketGCCharacterUpdate i publikuje zdarzenie CharacterUpdateEvent.
         * 
         * @param payload Bufor bajtow zawierajacy dane pakietu.
         * @return EterBase::PacketResult<void> ze statusem pomyslnym lub bledem.
         */
        static EterBase::PacketResult<void> HandleCharacterUpdate(std::span<const uint8_t> payload)
        {
            if (payload.size() < sizeof(TPacketGCCharacterUpdate))
            {
                EterBase::ModernLogger::Error("ActorPacket_CharUpdate::HandleCharacterUpdate: Buffer underflow. Expected {}, got {}", 
                    sizeof(TPacketGCCharacterUpdate), payload.size());
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            TPacketGCCharacterUpdate packet;
            std::memset(&packet, 0, sizeof(packet));
            std::memcpy(&packet, payload.data(), sizeof(packet));

            CharacterUpdateEvent event(packet);
            Core::EventBus::GetInstance().Publish(event);

            EterBase::ModernLogger::Debug("ActorPacket_CharUpdate::HandleCharacterUpdate: Processed update for entityId: {}", packet.dwVID);

            return {};
        }
    };
}
