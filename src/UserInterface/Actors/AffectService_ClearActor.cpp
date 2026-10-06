#include "../StdAfx.h"
#include "ICharacterAffectService.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <unordered_map>
#include <unordered_set>
#include <expected>

namespace UserInterface::Actors
{
    namespace Events
    {
        /**
         * @brief Zdarzenie emitowane po wyczyszczeniu wszystkich afektow postaci (np. przy smierci).
         */
        struct AffectsClearedEvent : public Core::IEvent
        {
            EterBase::EntityId entityId;

            /**
             * @brief Konstruktor zdarzenia.
             * @param id Identyfikator postaci.
             */
            explicit AffectsClearedEvent(EterBase::EntityId id) : entityId(id) {}
        };
    }

    /**
     * @brief Implementacja serwisu afektow ze wsparciem dla czyszczenia po smierci.
     */
    class CharacterAffectService final : public ICharacterAffectService
    {
    public:
        CharacterAffectService() = default;
        ~CharacterAffectService() override = default;

        /**
         * @brief Ustawia afekt na zadanej postaci.
         * @param id Identyfikator postaci.
         * @param affectIndex Indeks afektu.
         * @param enabled Czy aktywowac.
         */
        void SetAffect(EterBase::EntityId id, uint32_t affectIndex, bool enabled) override
        {
            if (enabled)
            {
                m_affects[id].insert(affectIndex);
                EterBase::ModernLogger::Debug("CharacterAffectService::SetAffect - Wlaczono afekt {} dla {}", affectIndex, id.value());
            }
            else
            {
                if (auto it = m_affects.find(id); it != m_affects.end())
                {
                    it->second.erase(affectIndex);
                    if (it->second.empty())
                    {
                        m_affects.erase(it);
                    }
                    EterBase::ModernLogger::Debug("CharacterAffectService::SetAffect - Wylaczono afekt {} dla {}", affectIndex, id.value());
                }
            }
        }

        /**
         * @brief Sprawdza czy postac posiada dany afekt.
         * @param id Identyfikator postaci.
         * @param affectIndex Indeks afektu.
         * @return Prawda jesli posiada afekt.
         */
        bool HasAffect(EterBase::EntityId id, uint32_t affectIndex) const override
        {
            auto it = m_affects.find(id);
            if (it != m_affects.end())
            {
                return it->second.contains(affectIndex);
            }
            return false;
        }

        /**
         * @brief Usuwa wszystkie afekty (np. podczas smierci postaci)
         * @param id Identyfikator postaci.
         */
        void ClearAffects(EterBase::EntityId id) override
        {
            auto result = InternalClearAffects(id);
            if (!result)
            {
                EterBase::ModernLogger::Debug("CharacterAffectService::ClearAffects - Postac {} nie miala afektow ({})", 
                    id.value(), EterBase::ToString(result.error()));
            }
        }

        /**
         * @brief Usuwa caly stan z serwisu.
         */
        void Clear() override
        {
            m_affects.clear();
            EterBase::ModernLogger::Info("CharacterAffectService::Clear - Wyczyszczono stan afektow");
        }

    private:
        /**
         * @brief Wewnetrzna metoda do czyszczenia afektow zwracajaca std::expected.
         * @param id Identyfikator postaci
         * @return std::expected z void lub typem bledu domenowego.
         */
        std::expected<void, EterBase::EntityError> InternalClearAffects(EterBase::EntityId id)
        {
            if (m_affects.erase(id) > 0)
            {
                EterBase::ModernLogger::Info("CharacterAffectService::InternalClearAffects - Usunieto afekty dla {}", id.value());
                Core::EventBus::GetInstance().Publish(Events::AffectsClearedEvent{id});
                return {};
            }
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        std::unordered_map<EterBase::EntityId, std::unordered_set<uint32_t>> m_affects;
    };
}
