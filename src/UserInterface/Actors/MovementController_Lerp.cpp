#include "../StdAfx.h"
#include "../Packet.h"
#include <cstdint>
#include <expected>
#include <cmath>
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../Core/EventBus.h"
#include "../Core/MovementEvents.h"

namespace UserInterface::Actors
{
    /**
     * @class MovementControllerLerp
     * @brief Odpowiada za plynna interpolacje liniowa (Lerp) pozycji aktora sieciowego miedzy pakietami.
     */
    class MovementControllerLerp final
    {
    public:
        MovementControllerLerp() = default;
        ~MovementControllerLerp() = default;

        /**
         * @brief Aktualizuje pozycje aktora za pomoca interpolacji liniowej miedzy pozycja startowa a docelowa.
         * 
         * @param entityId Identyfikator aktora.
         * @param startX Pozycja startowa X.
         * @param startY Pozycja startowa Y.
         * @param destX Pozycja docelowa X.
         * @param destY Pozycja docelowa Y.
         * @param progress Postep interpolacji (zakres od 0.0f do 1.0f).
         * @param rotation Opcjonalna rotacja postaci z pakietu.
         * @return EterBase::PacketResult<void> Zwraca blad jesli postep jest poza zakresem, inaczej sukces.
         */
        [[nodiscard]] EterBase::PacketResult<void> UpdateLerp(
            EterBase::EntityId entityId, 
            int32_t startX, int32_t startY, 
            int32_t destX, int32_t destY, 
            float progress,
            std::optional<uint8_t> rotation = std::nullopt) noexcept
        {
            if (!entityId)
            {
                EterBase::ModernLogger::Error("MovementControllerLerp: Brak prawidlowego EntityId!");
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            if (progress < 0.0f || progress > 1.0f)
            {
                EterBase::ModernLogger::Error("MovementControllerLerp: Postep interpolacji poza zakresem [0.0, 1.0]: {}", progress);
                return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
            }

            int32_t currentX = static_cast<int32_t>(std::round(std::lerp(static_cast<float>(startX), static_cast<float>(destX), progress)));
            int32_t currentY = static_cast<int32_t>(std::round(std::lerp(static_cast<float>(startY), static_cast<float>(destY), progress)));

            // Wyliczamy czy postac dotarla do celu (uwzgledniamy margines tolerancji ze wzgledu na float)
            bool reachedDestination = (progress >= 1.0f || (currentX == destX && currentY == destY));

            if (reachedDestination)
            {
                currentX = destX;
                currentY = destY;

                Core::Events::DestinationReached destEvent{entityId, currentX, currentY};
                if (auto result = destEvent.Validate(); result.has_value())
                {
                    Core::EventBus::GetInstance().Publish(destEvent);
                }
            }
            else
            {
                Core::Events::PlayerPositionUpdated posEvent{
                    entityId,
                    currentX,
                    currentY,
                    destX,
                    destY,
                    rotation
                };

                if (auto result = posEvent.Validate(); result.has_value())
                {
                    Core::EventBus::GetInstance().Publish(posEvent);
                }
            }

            return {};
        }
    };
} // namespace UserInterface::Actors
