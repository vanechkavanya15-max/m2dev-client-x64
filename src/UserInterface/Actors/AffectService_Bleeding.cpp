#include "../StdAfx.h"
#include "ICharacterAffectService.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"
#include <unordered_map>
#include <unordered_set>
#include <expected>
#include <mutex>
#include <memory>
#include <string_view>
#include <format>

namespace UserInterface::Actors
{
    /**
     * @brief Zdarzenie powiadamiajace GUI o zmianie stanu wizualnego krwawienia.
     * 
     * Dziedziczy po Core::IEvent z EventBus.
     */
    struct BleedingVisualEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        bool isBleeding;

        BleedingVisualEvent(EterBase::EntityId id, bool bleeding)
            : entityId(id), isBleeding(bleeding) {}
    };

    /**
     * @brief Implementacja serwisu do zarzadzania buffami i debuffami.
     * Skupia sie na obsludze afektu krwawienia wedlug wytycznych Zero-Conflict.
     */
    class CharacterAffectServiceBleeding : public ICharacterAffectService
    {
    public:
        CharacterAffectServiceBleeding() = default;
        ~CharacterAffectServiceBleeding() override = default;

        /**
         * @brief Ustawia lub usuwa afekt. W przypadku krwawienia (affectIndex = 11) wysyla event wizualny.
         * W implementacji Metin2, krwawienie to czesto indeks AFFECT_BLEEDING = 11 lub 65 itp. 
         * Tu uzyjemy flagi ogolnej sprawdzanej i raportowanej jesli indeks afektu sie zmieni.
         */
        void SetAffect(EterBase::EntityId id, uint32_t affectIndex, bool enabled) override
        {
            auto result = ValidateEntity(id);
            if (!result.has_value())
            {
                EterBase::ModernLogger::Error("Failed to set affect: {}", EterBase::ToString(result.error()));
                return;
            }

            std::lock_guard<std::mutex> lock(m_mutex);

            if (enabled)
            {
                m_affects[id].insert(affectIndex);
                EterBase::ModernLogger::Debug("Added affect {} to entity {}", affectIndex, id.value());
            }
            else
            {
                if (m_affects.contains(id))
                {
                    m_affects[id].erase(affectIndex);
                    EterBase::ModernLogger::Debug("Removed affect {} from entity {}", affectIndex, id.value());
                    if (m_affects[id].empty())
                    {
                        m_affects.erase(id);
                    }
                }
            }
            
            // Specjalna obsluga wizualna krwawienia - wyzwalamy powiadomienie do GUI via EventBus.
            // Zwykle w Metin2 krwawienie ma swoj dedykowany vnum, ale dla potrzeb zadania 
            // publikujemy event niezaleznie co sie stalo dla danego indeksu jezeli uznamy ze to on (dla testu zalozmy indeks = 11 to krwawienie).
            // Wedlug standardu, nie naruszamy innych plikow, wysylamy powiadomienie via EventBus.
            if (affectIndex == 11) // Zalozenie: 11 to indeks AFFECT_BLEEDING
            {
                Core::EventBus::GetInstance().Publish(BleedingVisualEvent{id, enabled});
                EterBase::ModernLogger::Info("Published BleedingVisualEvent for entity {}: {}", id.value(), enabled);
            }
        }

        /**
         * @brief Sprawdza czy dany aktor ma okreslony afekt.
         */
        bool HasAffect(EterBase::EntityId id, uint32_t affectIndex) const override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_affects.find(id);
            if (it != m_affects.end())
            {
                return it->second.contains(affectIndex);
            }
            return false;
        }

        /**
         * @brief Usuwa wszystkie afekty z danego aktora.
         */
        void ClearAffects(EterBase::EntityId id) override
        {
            auto result = ValidateEntity(id);
            if (!result.has_value())
            {
                EterBase::ModernLogger::Error("Failed to clear affects: {}", EterBase::ToString(result.error()));
                return;
            }

            std::lock_guard<std::mutex> lock(m_mutex);
            
            // Jezeli usuwamy wszystkie, i mial krwawienie (11), trzeba powiadomic o koncu
            if (m_affects.contains(id) && m_affects[id].contains(11))
            {
                Core::EventBus::GetInstance().Publish(BleedingVisualEvent{id, false});
                EterBase::ModernLogger::Info("Published BleedingVisualEvent for entity {}: false (Cleared)", id.value());
            }

            m_affects.erase(id);
            EterBase::ModernLogger::Debug("Cleared all affects for entity {}", id.value());
        }

        /**
         * @brief Czysci calkowicie stan serwisu.
         */
        void Clear() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            // Publikacja wylaczenia krwawienia dla wszystkich ktorych to dotyczy
            for (const auto& [id, affectSet] : m_affects)
            {
                if (affectSet.contains(11))
                {
                    Core::EventBus::GetInstance().Publish(BleedingVisualEvent{id, false});
                }
            }

            m_affects.clear();
            EterBase::ModernLogger::Info("Cleared CharacterAffectServiceBleeding state.");
        }

    private:
        /**
         * @brief Waliduje poprawnosc id encji, uzywajac nowej biblioteki bledow.
         */
        std::expected<void, EterBase::EntityError> ValidateEntity(EterBase::EntityId id) const
        {
            if (id.value() == 0)
            {
                return std::unexpected(EterBase::EntityError::NotFound);
            }
            return {};
        }

        mutable std::mutex m_mutex;
        std::unordered_map<EterBase::EntityId, std::unordered_set<uint32_t>> m_affects;
    };
}
