#include "../../StdAfx.h"

#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include <span>
#include <cstdint>

namespace Network::Events
{
    /**
     * @brief Zdarzenie sieciowe: Ustalenie lub dodanie celu lotu dla pocisku (np. strzala, kula ognia).
     * Opublikowanie tego zdarzenia w EventBus pozwala uniknac bezposredniego wolania funkcji GUI (np. GetGraphicThingInstancePtr).
     */
    struct FlyTargetEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId shooterId;
        EterBase::EntityId targetId;
        int32_t x;
        int32_t y;
        bool isAdd;

        FlyTargetEvent(EterBase::EntityId shooter, EterBase::EntityId target, int32_t posX, int32_t posY, bool add)
            : shooterId(shooter), targetId(target), x(posX), y(posY), isAdd(add) {}
    };

    /**
     * @brief Zdarzenie sieciowe: Stworzenie pocisku (efektu lotu) pomiedzy dwiema instancjami.
     */
    struct CreateFlyEvent : public UserInterface::Core::IEvent
    {
        uint8_t type;
        EterBase::EntityId startId;
        EterBase::EntityId endId;

        CreateFlyEvent(uint8_t flyType, EterBase::EntityId start, EterBase::EntityId end)
            : type(flyType), startId(start), endId(end) {}
    };
}

namespace Network::Dispatchers
{
    /**
     * @brief Odbior pocisku lecacego w strone celu (strzala, kula ognia) - celowanie.
     * 
     * @param buffer Bufor bajtow zawierajacy pakiet TPacketGCFlyTargeting.
     * @param isAdd Okresla czy to jest ADD_FLY_TARGETING czy tylko FLY_TARGETING.
     * @return EterBase::PacketResult<void> sukces, albo blad bufora/pakietu.
     */
    EterBase::PacketResult<void> ProcessFlyTargeting(std::span<const uint8_t> buffer, bool isAdd)
    {
        if (buffer.size() < sizeof(TPacketGCFlyTargeting))
        {
            EterBase::ModernLogger::Error("CombatDispatcher_FlyTarget: Buffer underflow in ProcessFlyTargeting (expected {}, got {})", 
                sizeof(TPacketGCFlyTargeting), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCFlyTargeting*>(buffer.data());

        EterBase::EntityId shooterId(packet->dwShooterVID);
        EterBase::EntityId targetId(packet->dwTargetVID);

        EterBase::ModernLogger::Debug("CombatDispatcher_FlyTarget: FlyTargeting [shooter: {}, target: {}, x: {}, y: {}, isAdd: {}]", 
            shooterId.value(), targetId.value(), packet->lX, packet->lY, isAdd);

        // Zgodnie z zasada zero-conflict oraz high-performance, powiadamiamy GUI za pomoca szyny zdarzen
        UserInterface::Core::EventBus::GetInstance().Publish(
            Events::FlyTargetEvent(shooterId, targetId, packet->lX, packet->lY, isAdd)
        );

        return {};
    }

    /**
     * @brief Odbior stworzenia wlasciwego efektu lotu (pocisku) w strone celu.
     * 
     * @param buffer Bufor bajtow zawierajacy pakiet TPacketGCCreateFly.
     * @return EterBase::PacketResult<void> sukces, albo blad bufora/pakietu.
     */
    EterBase::PacketResult<void> ProcessCreateFly(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCCreateFly))
        {
            EterBase::ModernLogger::Error("CombatDispatcher_FlyTarget: Buffer underflow in ProcessCreateFly (expected {}, got {})", 
                sizeof(TPacketGCCreateFly), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCCreateFly*>(buffer.data());

        EterBase::EntityId startId(packet->dwStartVID);
        EterBase::EntityId endId(packet->dwEndVID);

        EterBase::ModernLogger::Debug("CombatDispatcher_FlyTarget: CreateFly [type: {}, start: {}, end: {}]", 
            packet->bType, startId.value(), endId.value());

        // Zgodnie z zasada zero-conflict oraz high-performance, powiadamiamy GUI za pomoca szyny zdarzen
        UserInterface::Core::EventBus::GetInstance().Publish(
            Events::CreateFlyEvent(packet->bType, startId, endId)
        );

        return {};
    }
}
