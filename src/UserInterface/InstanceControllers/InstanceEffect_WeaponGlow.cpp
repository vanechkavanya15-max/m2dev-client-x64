#include "../StdAfx.h"
#include "IInstanceEffectController.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

#include <cstdint>
#include <string_view>

namespace UserInterface::InstanceControllers
{
    namespace
    {
        /**
         * @brief Zdarzenie emitowane po zmianie efektu swiecenia broni.
         * Oddziela logike od bezposrednich wywolan GUI.
         */
        struct WeaponGlowUpdateEvent : public UserInterface::Core::IEvent
        {
            EterBase::EntityId entityId;
            uint8_t partIndex;
            uint8_t refineLevel;
            uint32_t shineData;

            WeaponGlowUpdateEvent(EterBase::EntityId id, uint8_t part, uint8_t refine, uint32_t shine)
                : entityId(id), partIndex(part), refineLevel(refine), shineData(shine) {}
        };
    }

    /**
     * @brief Aktualizuje efekt swiecenia broni w zaleznosci od poziomu ulepszenia i danych shinedata.
     * 
     * @param effectController Kontroler efektow instancji.
     * @param entityId Identyfikator encji (VID).
     * @param partIndex Indeks czesci modelu (np. bron).
     * @param refineLevel Poziom ulepszenia (+7, +8, +9).
     * @param shineData Dodatkowe dane swiecenia (shinedata).
     * @return EterBase::PacketResult<void> Sukces operacji.
     */
    EterBase::PacketResult<void> UpdateWeaponGlow(
        IInstanceEffectController& effectController,
        EterBase::EntityId entityId,
        uint8_t partIndex,
        uint8_t refineLevel,
        uint32_t shineData)
    {
        EterBase::ModernLogger::Info(
            "UpdateWeaponGlow: Entity: {}, Part: {}, Refine: +{}, ShineData: {}",
            entityId.value(), partIndex, refineLevel, shineData
        );

        if (entityId.value() == 0)
        {
            EterBase::ModernLogger::Error("UpdateWeaponGlow: Niewazne ID encji.");
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        // W Metin2 glowne efekty swiecenia bronia pojawiaja sie dopiero od poziomu +7
        if (refineLevel > 0 && refineLevel < 7)
        {
            EterBase::ModernLogger::Debug(
                "UpdateWeaponGlow: Poziom ulepszenia {} nie aktywuje efektu swiecenia (+7 do +9).", 
                refineLevel
            );
            return {};
        }

        // Delegacja do glownego kontrolera efektow
        effectController.SetItemShine(partIndex, refineLevel);

        // Powiadomienie innych podsystemow / GUI o zmianie efektu swiecenia (Zero-Conflict)
        UserInterface::Core::EventBus::GetInstance().Publish(
            WeaponGlowUpdateEvent{entityId, partIndex, refineLevel, shineData}
        );

        return {};
    }
}
