#include "../StdAfx.h"
#include "IGroundDropBatchRenderer.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "Core/EventBus.h"

namespace UserInterface::GroundDrop
{
    /**
     * @brief Zdarzenie lokalne wysylane na szynę, gdy wykonano sprawdzenie zasiegu.
     * Zgodnie z zasada Zero-Conflict definiowane wylacznie wewnatrz .cpp.
     */
    struct DropInRangeCheckedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId virtualId;
        bool inRange;

        explicit DropInRangeCheckedEvent(EterBase::EntityId id, bool range)
            : virtualId(id), inRange(range) {}
    };

    /**
     * @brief Klasa odpowiedzialna za weryfikację czy gracz znajduje sie w zasiegu podniesienia danego przedmiotu.
     * Zgodnie z Single Responsibility Principle hermetyzuje te jedna operacje.
     */
    class GroundDropPickRangeChecker
    {
    public:
        /**
         * @brief Sprawdza czy gracz znajduje sie w zasięgu podniesienia przedmiotu z ziemi.
         * 
         * @param renderer Referencja do interfejsu dropu
         * @param virtualId Identyfikator instancji przedmiotu na ziemi (Strong Type)
         * @param playerX Pozycja X gracza
         * @param playerY Pozycja Y gracza
         * @param maxRange Maksymalny dystans dozwolony do podniesienia
         * @return EterBase::VoidResult<> (czyli std::expected<void, EntityError>)
         */
        static EterBase::Result<void, EterBase::EntityError> CheckRange(
            const IGroundDropBatchRenderer& renderer,
            EterBase::EntityId virtualId,
            float playerX,
            float playerY,
            float maxRange)
        {
            EterBase::ModernLogger::Debug("Weryfikacja zasiegu dla virtualId: {}", virtualId.value());

            bool isWithinRange = renderer.IsInRangeToPick(virtualId.value(), playerX, playerY, maxRange);

            // Publikacja zdarzenia do ewentualnych subskrybentów logiki wyższego rzędu
            DropInRangeCheckedEvent eventData{virtualId, isWithinRange};
            UserInterface::Core::EventBus::GetInstance().Publish(eventData);

            if (!isWithinRange)
            {
                EterBase::ModernLogger::Trace("Przedmiot virtualId: {} jest poza zasiegiem ({})", virtualId.value(), maxRange);
                return std::unexpected(EterBase::EntityError::OutOfRange);
            }

            EterBase::ModernLogger::Info("Przedmiot virtualId: {} znajduje sie w zasiegu", virtualId.value());
            return {};
        }
    };
} // namespace UserInterface::GroundDrop
