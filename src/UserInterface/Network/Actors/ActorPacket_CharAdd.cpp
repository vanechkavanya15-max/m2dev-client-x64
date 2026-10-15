#include "../../StdAfx.h"
#include "IActorNetworkDispatcher.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Packets/Packet_ActorAdd.h"
#include <cstring>

namespace UserInterface::Network
{
    /**
     * @brief Zdarzenie publikowane po odebraniu pakietu HEADER_GC_CHARACTER_ADD.
     * Zastępuje bezpośrednie wywołania Python UI.
     */
    struct ActorSpawnEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        float angle;
        int32_t x;
        int32_t y;
        int32_t z;
        uint8_t type;
        uint16_t raceNum;
        uint8_t movingSpeed;
        uint8_t attackSpeed;
        uint8_t stateFlag;
        uint32_t affectFlag[2];
    };

    /**
     * @brief Izolowana, bezkonfliktowa implementacja obslugi pakietu HEADER_GC_CHARACTER_ADD.
     * Realizuje zasade Single Responsibility, zglaszajac zdarzenia dla innych systemow.
     */
    class ActorNetworkDispatcher_CharAdd final : public IActorNetworkDispatcher
    {
    public:
        ActorNetworkDispatcher_CharAdd() = default;
        ~ActorNetworkDispatcher_CharAdd() override = default;

        /**
         * @brief Parsuje pakiet i publikuje zdarzenie.
         * @param payload Bufor z danymi pakietu.
         * @return Sukces lub blad parsowania.
         */
        EterBase::PacketResult<void> HandleCharacterAdd(std::span<const uint8_t> payload) override
        {
            if (payload.size() < sizeof(TPacketGCCharacterAdd))
            {
                EterBase::ModernLogger::Error("ActorNetworkDispatcher_CharAdd: Buffer underflow. Expected {}, got {}",
                                              sizeof(TPacketGCCharacterAdd), payload.size());
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const TPacketGCCharacterAdd*>(payload.data());

            ActorSpawnEvent event{};
            event.entityId = EterBase::EntityId(packet->id);
            event.angle = packet->angle;
            event.x = packet->x;
            event.y = packet->y;
            event.z = packet->z;
            event.type = packet->type;
            event.raceNum = packet->raceNum;
            event.movingSpeed = packet->movingSpeed;
            event.attackSpeed = packet->attackSpeed;
            event.stateFlag = packet->stateFlag;
            event.affectFlag[0] = packet->affectFlag[0];
            event.affectFlag[1] = packet->affectFlag[1];

            EterBase::ModernLogger::Debug("ActorNetworkDispatcher_CharAdd: Spawned EntityId={}, Race={}", packet->id, packet->raceNum);

            Core::EventBus::GetInstance().Publish(event);

            return {};
        }

        // --- Puste zasilepki dla innych metod interfejsu (Zero-Conflict) ---
        EterBase::PacketResult<void> HandleCharacterUpdate(std::span<const uint8_t>) override { return {}; }
        EterBase::PacketResult<void> HandleCharacterDelete(std::span<const uint8_t>) override { return {}; }
        EterBase::PacketResult<void> HandleCharacterMove(std::span<const uint8_t>) override { return {}; }
        EterBase::PacketResult<void> HandleDamageInfo(std::span<const uint8_t>) override { return {}; }
        EterBase::PacketResult<void> HandleCharacterDie(std::span<const uint8_t>) override { return {}; }
        void Clear() override {}
    };
}
