#include "../StdAfx.h"
#include "TransformComponentTable.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"
#include <expected>
#include <string>

namespace UserInterface::ECS
{
    namespace
    {
        /**
         * @brief Lokalny event powiadamiajacy o zainicjalizowaniu tabeli Transform.
         * Hermetyzacja wewnatrz pliku chroni przed konfliktami (zero-conflict rule).
         * Umieszczony w anonimowej przestrzeni nazw dla wewnetrznego linkowania.
         */
        struct TransformTableInitializedEvent : public UserInterface::Core::IEvent
        {
            size_t capacity;

            explicit TransformTableInitializedEvent(size_t capacity) : capacity(capacity) {}
        };
    }

    /**
     * @brief Inicjalizuje i alokuje pamiec dla tabeli Transform (wyrownanie pod wektoryzacje AVX2).
     * @param table Referencja do tabeli TransformComponentTable.
     * @param capacity Ilosc rezerwowanej pojemnosci.
     * @return std::expected<void, std::string> informujacy o sukcesie lub bledzie.
     */
    std::expected<void, std::string> InitTransformTable(TransformComponentTable& table, size_t capacity)
    {
        if (capacity == 0)
        {
            EterBase::ModernLogger::Error("Failed to initialize TransformComponentTable: capacity cannot be 0");
            return std::unexpected("Capacity cannot be 0");
        }

        try
        {
            table.Reserve(capacity);
            
            EterBase::ModernLogger::Info("TransformComponentTable initialized and reserved with capacity: {}", capacity);

            TransformTableInitializedEvent event(capacity);
            UserInterface::Core::EventBus::GetInstance().Publish(event);

            return {};
        }
        catch (const std::exception& e)
        {
            EterBase::ModernLogger::Error("Exception occurred while initializing TransformComponentTable: {}", e.what());
            return std::unexpected(std::string("Allocation failed: ") + e.what());
        }
    }
}
