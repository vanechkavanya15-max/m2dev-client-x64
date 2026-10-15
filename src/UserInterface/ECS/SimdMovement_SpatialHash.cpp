#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "../Packet.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <vector>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <span>

namespace UserInterface::ECS
{
    /**
     * @brief Zdarzenie informujace o pomyslnym wygenerowaniu kluczy przestrzennych.
     */
    struct SpatialHashUpdatedEvent : public Core::IEvent
    {
        /** @brief Liczba zaktualizowanych encji */
        size_t updatedEntitiesCount;
        
        /**
         * @brief Konstruktor inicjalizujacy zdarzenie.
         * @param count Liczba zaktualizowanych encji.
         */
        explicit SpatialHashUpdatedEvent(size_t count) : updatedEntitiesCount(count) {}
    };

    /**
     * @brief Klasa odpowiedzialna za generowanie kluczy siatki przestrzennej.
     * 
     * Przystosowana do wektoryzacji SIMD/AVX2 dla szybkiej partycji swiata gier
     * w architekturze ECS.
     */
    class SimdMovement_SpatialHash
    {
    public:
        /** @brief Rozmiar pojedynczej komorki siatki przestrzennej w swiecie. */
        static constexpr float GRID_CELL_SIZE = 2500.0f;

        /**
         * @brief Oblicza klucz siatki przestrzennej dla zadanej pozycji (X, Y).
         * 
         * @param x Wspolrzedna X encji w swiecie gry.
         * @param y Wspolrzedna Y encji w swiecie gry.
         * @return uint64_t 64-bitowy wygenerowany klucz przestrzenny.
         */
        [[nodiscard]] static constexpr uint64_t CalculateHash(float x, float y) noexcept
        {
            const int32_t cellX = static_cast<int32_t>(x / GRID_CELL_SIZE);
            const int32_t cellY = static_cast<int32_t>(y / GRID_CELL_SIZE);
            
            return (static_cast<uint64_t>(static_cast<uint32_t>(cellX)) << 32) | static_cast<uint32_t>(cellY);
        }

        /**
         * @brief Generuje klucze siatki przestrzennej na podstawie komponentow transformacji (SoA).
         * 
         * Wykonuje przeliczenie i zapisuje klucze w wektorze wyjsciowym. Na koniec emituje
         * zdarzenie informujace o zakonczeniu operacji.
         * 
         * @param table Tabela komponentow transformacji zawierajaca pozycje encji.
         * @param outHashes Wektor wyjsciowy, do ktorego zostana zapisane wyniki (zorientowany SoA).
         * @return EterBase::VoidResult<> Informacja o sukcesie (lub błąd w przypadku wyjatku).
         */
        static EterBase::VoidResult<> GenerateHashes(const TransformComponentTable& table, std::vector<uint64_t>& outHashes)
        {
            const size_t entityCount = table.Size();
            if (entityCount == 0)
            {
                return {};
            }

            try
            {
                outHashes.resize(entityCount);
                
                const float* posXArray = table.posX.data();
                const float* posYArray = table.posY.data();
                uint64_t* hashArray = outHashes.data();

                // Petla SoA (Structure of Arrays), zoptymalizowana pod SIMD
                for (size_t i = 0; i < entityCount; ++i)
                {
                    hashArray[i] = CalculateHash(posXArray[i], posYArray[i]);
                }

                EterBase::ModernLogger::Info("SimdMovement_SpatialHash: Wygenerowano hasze przestrzenne dla {} encji.", entityCount);
                
                // Emisja zdarzenia do szyny EventBus (zgodnie z architekturą decoupingu od GUI)
                Core::EventBus::GetInstance().Publish(SpatialHashUpdatedEvent(entityCount));

                return {};
            }
            catch (const std::exception& ex)
            {
                EterBase::ModernLogger::Error("SimdMovement_SpatialHash: Blad podczas generowania kluczy: {}", ex.what());
                return EterBase::MakeError(std::string_view("Wyjatek podczas generowania kluczy"));
            }
        }
    };
}
