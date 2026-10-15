#include "../StdAfx.h"
#include "IDropPhysicsSimulator.h"
#include "EterBase/LogModern.h"
#include "EterBase/StrongTypes.h"
#include "UserInterface/Core/EventBus.h"

#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <expected>
#include <memory>

namespace UserInterface::GroundDrop
{
    /**
     * @brief Zdarzenie publikowane na szynie EventBus, gdy wyrzucony przedmiot osiadzie na ziemi.
     */
    struct GroundDropSettledEvent : public Core::IEvent
    {
        EterBase::EntityId virtualId;
        float finalX;
        float finalY;
        float finalZ;

        GroundDropSettledEvent(EterBase::EntityId id, float x, float y, float z)
            : virtualId(id), finalX(x), finalY(y), finalZ(z) {}
    };

    /**
     * @brief Implementacja symulatora fizyki opadania przedmiotow ze sprezystym odbiciem.
     * 
     * Odpowiada za obliczanie trajektorii i grawitacji. Po uderzeniu w ziemie, 
     * przedmiot odbija sie z tlumieniem predkosci, az do calkowitego zatrzymania.
     */
    class GroundBouncePhysicsSimulator final : public IDropPhysicsSimulator
    {
    public:
        GroundBouncePhysicsSimulator() = default;
        ~GroundBouncePhysicsSimulator() override = default;

        /**
         * @brief Spawnuje impuls poczatkowy dla wyrzucanego przedmiotu.
         * @param virtualId Identyfikator wirtualny (VID) przedmiotu.
         * @param originX Poczatkowa pozycja X.
         * @param originY Poczatkowa pozycja Y.
         * @param originZ Poczatkowa pozycja Z.
         * @param force Sila odrzutu.
         */
        void SpawnImpulse(uint32_t virtualId, float originX, float originY, float originZ, float force) override
        {
            DropTrajectory traj;
            traj.posX = originX;
            traj.posY = originY;
            traj.posZ = originZ;
            
            // Generujemy pseudolosowy kierunek odrzutu oparty na virtualId, aby zachowac determinizm.
            float angle = static_cast<float>(virtualId % 360) * 3.14159f / 180.0f;
            traj.velX = std::cos(angle) * force;
            traj.velY = std::sin(angle) * force;
            traj.velZ = force * 1.5f; // Sila wzbijajaca
            traj.isSettled = false;

            m_trajectories[virtualId] = traj;
            EterBase::ModernLogger::Debug("Spawned impulse for drop id {} at {}, {}, {}", virtualId, originX, originY, originZ);
        }

        /**
         * @brief Krok symulacji fizyki.
         * @param deltaTime Czas, jaki uplynal od ostatniej klatki.
         * @param groundHeight Wysokosc terenu (Z) w miejscu upadku (w uproszczeniu, traktujemy plasko).
         */
        void StepPhysics(float deltaTime, float groundHeight) override
        {
            if (deltaTime <= 0.0f)
                return;

            const float gravity = -980.0f; // Przykladowe stalej grawitacji
            const float bounceDamping = 0.5f;
            const float frictionDamping = 0.8f;
            const float settleThreshold = 10.0f;

            for (auto& [id, traj] : m_trajectories)
            {
                if (traj.isSettled)
                    continue;

                traj.velZ += gravity * deltaTime;

                traj.posX += traj.velX * deltaTime;
                traj.posY += traj.velY * deltaTime;
                traj.posZ += traj.velZ * deltaTime;

                if (traj.posZ <= groundHeight)
                {
                    traj.posZ = groundHeight;
                    
                    if (std::abs(traj.velZ) < settleThreshold)
                    {
                        traj.isSettled = true;
                        traj.velX = 0.0f;
                        traj.velY = 0.0f;
                        traj.velZ = 0.0f;
                        
                        Core::EventBus::GetInstance().Publish(
                            GroundDropSettledEvent(EterBase::EntityId(id), traj.posX, traj.posY, traj.posZ)
                        );
                        
                        EterBase::ModernLogger::Debug("Drop {} settled at {}, {}, {}", id, traj.posX, traj.posY, traj.posZ);
                    }
                    else
                    {
                        traj.velZ = -traj.velZ * bounceDamping;
                        traj.velX *= frictionDamping;
                        traj.velY *= frictionDamping;
                    }
                }
            }
        }

        /**
         * @brief Pobiera aktualna trajektorie rzuconego przedmiotu.
         * @param virtualId Identyfikator wirtualny (VID) przedmiotu.
         * @return Struktura trajektorii.
         */
        DropTrajectory GetTrajectory(uint32_t virtualId) const override
        {
            if (auto it = m_trajectories.find(virtualId); it != m_trajectories.end())
            {
                return it->second;
            }
            return DropTrajectory{};
        }

        /**
         * @brief Czysci wszystkie trajektorie.
         */
        void Clear() override
        {
            m_trajectories.clear();
        }

    private:
        std::unordered_map<uint32_t, DropTrajectory> m_trajectories;
    };
}
