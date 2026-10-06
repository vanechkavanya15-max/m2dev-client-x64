#include "../StdAfx.h"
#include "IInstanceMountHorseController.h"
#include "IInstanceAnimationController.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <expected>

namespace UserInterface::InstanceControllers
{
    /**
     * @brief Zdarzenie lokalne informujace o zsynchronizowaniu animacji miedzy wierzchowcem a jezdzcem.
     * Zgodnie z zasada Zero-Conflict definiowane lokalnie.
     */
    struct MountAnimSyncedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId riderId;
        uint32_t motionKey;

        MountAnimSyncedEvent(EterBase::EntityId rider, uint32_t motion)
            : riderId(rider), motionKey(motion) {}
    };

    /**
     * @brief Standalone class (Zasada zero-conflict) realizujaca wspolbiezne odtwarzanie animacji stepa/klusa konia oraz dopasowanych ruchow tulowia gracza.
     */
    class MountAnimLinker
    {
    public:
        /**
         * @brief Synchronizuje animacje miedzy jezdzcem i wierzchowcem.
         * 
         * @param mountController Kontroler stanu wierzchowca.
         * @param horseAnimCtrl Kontroler animacji konia.
         * @param riderAnimCtrl Kontroler animacji gracza.
         * @param motionKey Klucz animacji do odtworzenia (np. step lub klus).
         * @param speedMultiplier Mnoznik predkosci animacji.
         * @return EterBase::PacketResult<void> Sukces lub blad w przypadku braku mounta.
         */
        static EterBase::PacketResult<void> SyncAnimations(
            const IInstanceMountHorseController& mountController,
            IInstanceAnimationController& horseAnimCtrl,
            IInstanceAnimationController& riderAnimCtrl,
            uint32_t motionKey,
            float speedMultiplier = 1.0f)
        {
            if (!mountController.IsMounted())
            {
                EterBase::ModernLogger::Warning("MountAnimLinker: Player is not mounted, cannot sync animations.");
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            EterBase::EntityId riderId = mountController.GetMountVID(); // Assuming GetMountVID returns the rider/mount identifier used for logic
            
            MotionConfig horseConfig;
            horseConfig.motionKey = motionKey;
            horseConfig.blendTime = 0.15f;
            horseConfig.speedMultiplier = speedMultiplier;
            horseConfig.isLooping = true;

            MotionConfig riderConfig = horseConfig; // Rider follows the same config base

            // Odpalenie animacji wierzchowca
            auto horseResult = horseAnimCtrl.PlayMotion(horseConfig);
            if (!horseResult.has_value())
            {
                EterBase::ModernLogger::Error("MountAnimLinker: Failed to play motion on horse.");
                return horseResult;
            }

            // Odpalenie dopasowanej animacji tulowia gracza
            auto riderResult = riderAnimCtrl.PlayMotion(riderConfig);
            if (!riderResult.has_value())
            {
                EterBase::ModernLogger::Error("MountAnimLinker: Failed to play synced motion on rider.");
                return riderResult;
            }

            EterBase::ModernLogger::Info("MountAnimLinker: Synced animation {} for rider {}", motionKey, riderId.get());

            // Publikacja zdarzenia na szynie EventBus (zero-conflict)
            MountAnimSyncedEvent event(riderId, motionKey);
            Core::EventBus::GetInstance().Publish(event);

            return {};
        }
    };
} // namespace UserInterface::InstanceControllers
