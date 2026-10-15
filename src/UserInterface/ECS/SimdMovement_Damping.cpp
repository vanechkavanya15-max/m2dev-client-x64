#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include <expected>
#include <cmath>
#include <algorithm>

namespace UserInterface::ECS
{
    /**
     * @brief Zdarzenie wyzwalane gdy encja zakonczy swoj ruch knockback (odrzut).
     */
    struct KnockbackEndedEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId entityId;

        explicit KnockbackEndedEvent(EterBase::EntityId id) : entityId(id) {}
    };

    /**
     * @brief System odpowiedzialny za wygaszanie wektora odrzutu (knockback) w czasie.
     */
    class SimdMovementDampingSystem final
    {
    public:
        /**
         * @brief Aktualizuje predkosci encji, aplikujac damping.
         * @param transformTable Referencja do tabeli TransformComponentTable (SoA).
         * @param deltaTime Czas od ostatniej klatki w sekundach.
         * @param dampingFactor Wspolczynnik wygaszania.
         * @return std::expected<void, EterBase::EntityError> z wynikiem operacji.
         */
        static std::expected<void, EterBase::EntityError> UpdateKnockbackDamping(
            TransformComponentTable& transformTable,
            float deltaTime,
            float dampingFactor = 5.0f)
        {
            if (deltaTime <= 0.0f)
            {
                return std::unexpected(EterBase::EntityError::None);
            }

            if (transformTable.Size() == 0)
            {
                return {};
            }

            size_t count = transformTable.Size();
            auto& eventBus = UserInterface::Core::EventBus::GetInstance();
            
            size_t stoppedCount = 0;

            for (size_t i = 0; i < count; ++i)
            {
                float& speed = transformTable.velocity[i];
                if (speed > 0.0f)
                {
                    // Aplikuj damping liniowy / tarcie
                    speed -= dampingFactor * deltaTime;

                    if (speed <= 0.0f)
                    {
                        speed = 0.0f;
                        
                        EterBase::EntityId eId(transformTable.entityIds[i]);
                        eventBus.Publish(KnockbackEndedEvent{eId});
                        ++stoppedCount;
                    }
                }
                else if (speed < 0.0f)
                {
                    // W przypadku ujemnych predkosci (bledy zaokraglen lub nienaturalny stan) wyzeruj
                    speed = 0.0f;
                }
            }
            
            if (stoppedCount > 0)
            {
                EterBase::ModernLogger::Debug("Zakonczono knockback dla {} encji.", stoppedCount);
            }

            return {};
        }
    };
}
