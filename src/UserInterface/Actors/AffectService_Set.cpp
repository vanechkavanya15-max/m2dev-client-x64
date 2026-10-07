#include "../StdAfx.h"
#include "ICharacterAffectService.h"
#include "../../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../Packet.h"

#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <span>

namespace UserInterface::Actors
{
    /**
     * @brief Zdarzenie emitowane, gdy stan afektu (dodanie/usuniecie) ulega zmianie.
     */
    struct AffectStateChangedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;
        uint32_t affectIndex;
        bool isEnabled;

        AffectStateChangedEvent(EterBase::EntityId id, uint32_t affect, bool enabled)
            : entityId(id), affectIndex(affect), isEnabled(enabled) {}
    };

    /**
     * @brief Zdarzenie emitowane, gdy wszystkie afekty podmiotu zostana wyczyszczone.
     */
    struct AffectsClearedEvent : public Core::IEvent
    {
        EterBase::EntityId entityId;

        explicit AffectsClearedEvent(EterBase::EntityId id) : entityId(id) {}
    };

    /**
     * @brief Glowna implementacja uslug afektow z zachowaniem Single Responsibility Principle
     *        oraz pelna izolacja od GUI (publikacja przez EventBus).
     */
    class CharacterAffectService final : public ICharacterAffectService
    {
    public:
        CharacterAffectService() = default;
        ~CharacterAffectService() override = default;

        /**
         * @brief Ustawia stan afektu dla zadanego podmiotu.
         * @param id Identyfikator podmiotu (np. VID gracza lub potwora).
         * @param affectIndex Indeks afektu.
         * @param enabled Czy afekt jest wlaczony (true) czy wylaczony (false).
         */
        void SetAffect(EterBase::EntityId id, uint32_t affectIndex, bool enabled) override
        {
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                
                if (enabled)
                {
                    affects_[id.value()].insert(affectIndex);
                    EterBase::ModernLogger::Debug("Zalozono afekt: ID {}, AffectIndex {}", id.value(), affectIndex);
                }
                else
                {
                    auto it = affects_.find(id.value());
                    if (it != affects_.end())
                    {
                        it->second.erase(affectIndex);
                        if (it->second.empty())
                        {
                            affects_.erase(it);
                        }
                    }
                    EterBase::ModernLogger::Debug("Zdjeto afekt: ID {}, AffectIndex {}", id.value(), affectIndex);
                }
            }
            
            // Dekouplowane powiadomienie poprzez EventBus
            Core::EventBus::GetInstance().Publish(AffectStateChangedEvent(id, affectIndex, enabled));
        }

        /**
         * @brief Sprawdza, czy dany podmiot posiada aktywny afekt.
         * @param id Identyfikator podmiotu.
         * @param affectIndex Indeks afektu do sprawdzenia.
         * @return Zwraca true, jezeli afekt jest wlaczony.
         */
        bool HasAffect(EterBase::EntityId id, uint32_t affectIndex) const override
        {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto it = affects_.find(id.value());
            if (it != affects_.end())
            {
                return it->second.contains(affectIndex);
            }
            return false;
        }

        /**
         * @brief Czysci wszystkie afekty wybranego podmiotu.
         * @param id Identyfikator podmiotu.
         */
        void ClearAffects(EterBase::EntityId id) override
        {
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                affects_.erase(id.value());
                EterBase::ModernLogger::Info("Wyczyszczono wszystkie afekty: ID {}", id.value());
            }

            // Dekouplowane powiadomienie poprzez EventBus
            Core::EventBus::GetInstance().Publish(AffectsClearedEvent(id));
        }

        /**
         * @brief Czysci globalnie wszystkie afekty w pamieci.
         */
        void Clear() override
        {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            affects_.clear();
            EterBase::ModernLogger::Info("Globalnie wyczyszczono rejestr afektow.");
        }
        
        /**
         * @brief Przetwarza pakiet dodania afektu. Zgodnie ze standardem C++23 
         * zwraca EterBase::PacketResult<void> do bezpiecznej kontroli bledow.
         * Zwykle podmiotem jest glowny bohater gracza, stykajacy sie z logika Network/Handlers.
         */
        EterBase::PacketResult<void> HandleAffectAddPacket(EterBase::EntityId id, std::span<const uint8_t> buffer)
        {
            if (buffer.size() < sizeof(TPacketGCAffectAdd))
            {
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const TPacketGCAffectAdd*>(buffer.data());
            
            // Logika nadania afektu
            SetAffect(id, packet->elem.dwType, true);
            
            return {};
        }

        /**
         * @brief Przetwarza pakiet usuniecia afektu.
         */
        EterBase::PacketResult<void> HandleAffectRemovePacket(EterBase::EntityId id, std::span<const uint8_t> buffer)
        {
            if (buffer.size() < sizeof(TPacketGCAffectRemove))
            {
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const TPacketGCAffectRemove*>(buffer.data());
            
            // Logika zdjecia afektu
            SetAffect(id, packet->dwType, false);
            
            return {};
        }

    private:
        mutable std::shared_mutex mutex_;
        std::unordered_map<uint32_t, std::unordered_set<uint32_t>> affects_;
    };
 
    /**
     * @brief Fabryka uslugi umozliwiajaca utworzenie instancji z zachowaniem enkapsulacji (SOLID).
     */
    std::unique_ptr<ICharacterAffectService> CreateAffectService()
    {
        return std::make_unique<CharacterAffectService>();
    }
} // namespace UserInterface::Actors
