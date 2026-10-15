#include "../StdAfx.h"
#include "IDropPhysicsSimulator.h"
#include "EterBase/ModernLogger.h"
#include "EterBase/StrongTypes.h"
#include "UserInterface/Core/EventBus.h"
#include "UserInterface/Core/Events.h"
#include <unordered_map>
#include <cmath>
#include <memory>

namespace UserInterface::GroundDrop
{
    /**
     * @brief Symulator fizyki rozrzucania upuszczonych przedmiotow w promienisty pierscien (Scatter Ring).
     * 
     * Implementuje ruch promienisty z efektem wyrzutu w gore uzywany przy rozrzucaniu wielu
     * przedmiotow wokol bossa. Zapewnia brak konfliktow (Zero-Conflict Rule) oraz wysokosc
     * standardow C++23.
     */
    class ScatterRingSimulator final : public IDropPhysicsSimulator
    {
    public:
        ScatterRingSimulator() = default;
        ~ScatterRingSimulator() override = default;

        /**
         * @brief Spawnuje impuls dla przedmiotu upuszczonego na ziemie.
         * @param virtualId Unikalny identyfikator instancji przedmiotu (VID).
         * @param originX Poczatkowa pozycja X.
         * @param originY Poczatkowa pozycja Y.
         * @param originZ Poczatkowa pozycja Z.
         * @param force Sila wyrzutu.
         */
        void SpawnImpulse(uint32_t virtualId, float originX, float originY, float originZ, float force) override
        {
            EterBase::EntityId entityId(virtualId);
            
            DropTrajectory traj{};
            traj.posX = originX;
            traj.posY = originY;
            traj.posZ = originZ;

            // Zloty kat (okolo 137.5 stopnia) zapewnia rownomierne rozlozenie 
            // kropli w pierscieniu dla kolejnych indeksow bez znajomosci ich z gory.
            constexpr float GOLDEN_ANGLE = 2.39996323f;
            float angle = static_cast<float>(m_spawnCount) * GOLDEN_ANGLE;

            // Obliczamy wektory predkosci dla ruchu promienistego
            float horizontalForce = force * 1.5f; 
            traj.velX = horizontalForce * std::cos(angle);
            traj.velY = horizontalForce * std::sin(angle);
            
            // Wyrzut w gore z uzyciem poczatkowej sily
            traj.velZ = force * 2.0f;
            traj.isSettled = false;

            m_trajectories[entityId] = traj;
            m_spawnCount++;

            EterBase::ModernLogger::Debug("ScatterRingSimulator: SpawnImpulse for item VID {}, origin ({}, {}, {}) force {}",
                entityId.value(), originX, originY, originZ, force);
        }

        /**
         * @brief Aktualizuje fizyke (pozycje, predkosc) wszystkich aktywnych przedmiotow.
         * @param deltaTime Czas od ostatniej klatki.
         * @param groundHeight Wysokosc terenu (Z), na ktorej obiekty maja sie zatrzymac.
         */
        void StepPhysics(float deltaTime, float groundHeight) override
        {
            constexpr float GRAVITY = 980.0f;

            for (auto& [entityId, traj] : m_trajectories)
            {
                if (traj.isSettled)
                    continue;

                // Aplikuj grawitacje do predkosci pionowej
                traj.velZ -= GRAVITY * deltaTime;

                // Aktualizuj pozycje na podstawie predkosci
                traj.posX += traj.velX * deltaTime;
                traj.posY += traj.velY * deltaTime;
                traj.posZ += traj.velZ * deltaTime;

                // Sprawdz kolizje z ziemia
                if (traj.posZ <= groundHeight && traj.velZ < 0.0f)
                {
                    traj.posZ = groundHeight;
                    traj.velX = 0.0f;
                    traj.velY = 0.0f;
                    traj.velZ = 0.0f;
                    traj.isSettled = true;
                    
                    // Powiadom system o opadnieciu przedmiotu
                    ::Core::Events::ItemDrop dropEvent{};
                    dropEvent.id = entityId.value();
                    dropEvent.vnum = 0; // Vnum nie jest tutaj dostepne na poziomie fizyki
                    dropEvent.count = 0; // Count rowniez, wysylamy tylko zeby zaktualizowac UI
                    dropEvent.x = static_cast<int32_t>(traj.posX);
                    dropEvent.y = static_cast<int32_t>(traj.posY);
                    dropEvent.z = static_cast<int32_t>(traj.posZ);
                    
                    UserInterface::Core::EventBus::GetInstance().Publish(dropEvent);
                }
            }
        }

        /**
         * @brief Pobiera aktualny stan trajektorii dla okreslonego przedmiotu.
         * @param virtualId Identyfikator instancji przedmiotu.
         * @return Struktura DropTrajectory z aktualnymi wspolrzednymi i predkosciami.
         */
        DropTrajectory GetTrajectory(uint32_t virtualId) const override
        {
            EterBase::EntityId entityId(virtualId);
            if (auto it = m_trajectories.find(entityId); it != m_trajectories.end())
            {
                return it->second;
            }
            return DropTrajectory{};
        }

        /**
         * @brief Czysci stan wszystkich symulacji i resetuje licznik zrespionych kropli.
         */
        void Clear() override
        {
            m_trajectories.clear();
            m_spawnCount = 0;
            EterBase::ModernLogger::Debug("ScatterRingSimulator: Oczyszczono symulacje fizyki i zresetowano licznik");
        }

    private:
        std::unordered_map<EterBase::EntityId, DropTrajectory> m_trajectories;
        uint32_t m_spawnCount{0};
    };

    /**
     * @brief Fabryka do utworzenia symulatora Scatter Ring.
     * Exportujemy ja by inne moduly mogly inicjalizowac symulator nie wiedzac o jego klasie wewnetrznej.
     * 
     * @return Zwraca inteligentny wskaznik na symulator Drop Physics.
     */
    std::unique_ptr<IDropPhysicsSimulator> CreateScatterRingSimulator()
    {
        return std::make_unique<ScatterRingSimulator>();
    }
}
