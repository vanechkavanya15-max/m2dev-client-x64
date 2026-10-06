#include "../StdAfx.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../Packet.h"

#include <cmath>

namespace UserInterface::Actors {

    struct Position {
        float x, y, z;
    };

    /**
     * @brief Zdarzenie rozglaszane na szynie po udanym wypchnieciu aktora.
     */
    struct CollisionPushEvent : public Core::IEvent {
        EterBase::EntityId pusherId;
        EterBase::EntityId pushedId;
        Position newPushedPosition;

        CollisionPushEvent(EterBase::EntityId p1, EterBase::EntityId p2, Position pos)
            : pusherId(p1), pushedId(p2), newPushedPosition(pos) {}
    };

    /**
     * @brief Kontroler odpowiedzialny za obliczanie wypychania przy kolizjach
     * Zgodny ze standardem C++23, zero-copy (gdzie to mozliwe) i bez posrednich stanow GUI.
     */
    class MovementController_Push {
    public:
        /**
         * @brief Oblicza i wykonuje logike wypchniecia (push) z uzyciem EterBase::Result.
         * 
         * @param pusherId ID aktora wypychajacego
         * @param pusherPos Pozycja wypychajacego
         * @param pusherRadius Promien kolizji wypychajacego
         * @param pushedId ID aktora wypychanego
         * @param pushedPos Pozycja wypychanego
         * @param pushedRadius Promien kolizji wypychanego
         * @return Oczekiwana nowa pozycja wypychanego lub blad (np. OutOfRange gdy brak kolizji).
         */
        static EterBase::Result<Position, EterBase::EntityError> CalculatePush(
            EterBase::EntityId pusherId,
            const Position& pusherPos,
            float pusherRadius,
            EterBase::EntityId pushedId,
            const Position& pushedPos,
            float pushedRadius) 
        {
            if (pusherId == pushedId) {
                EterBase::ModernLogger::Error("CalculatePush: Aktor nie moze wypychac samego siebie (ID {})", pusherId.value());
                return EterBase::MakeError(EterBase::EntityError::InvalidType);
            }

            float dx = pushedPos.x - pusherPos.x;
            float dy = pushedPos.y - pusherPos.y;
            float distanceSq = dx * dx + dy * dy;
            float minDistance = pusherRadius + pushedRadius;

            // Sprawdzenie czy w ogole zaszla kolizja
            if (distanceSq >= minDistance * minDistance) {
                return EterBase::MakeError(EterBase::EntityError::OutOfRange); 
            }

            float distance = std::sqrt(distanceSq);
            
            float pushFactor = 0.0f;

            // Jesli obiekty sa w dokladnie tym samym miejscu, wybierz losowy wektor wypchniecia
            if (distance == 0.0f) {
                dx = 1.0f;
                dy = 0.0f;
                distance = 1.0f;
                pushFactor = minDistance;
            } else {
                pushFactor = minDistance - distance;
            }
            
            float nx = dx / distance;
            float ny = dy / distance;

            Position newPushedPos = {
                pushedPos.x + nx * pushFactor,
                pushedPos.y + ny * pushFactor,
                pushedPos.z
            };

            EterBase::ModernLogger::Info(
                "Calculated collision push for Entity {}, pushed to ({}, {})", 
                pushedId.value(), newPushedPos.x, newPushedPos.y
            );

            // Powiadomienie innych systemow (np. GUI) o zmianie pozycji z powodu wypchniecia
            Core::EventBus::GetInstance().Publish(CollisionPushEvent(pusherId, pushedId, newPushedPos));

            return newPushedPos;
        }
    };
} // namespace UserInterface::Actors
