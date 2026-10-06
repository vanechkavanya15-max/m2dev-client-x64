#include "../StdAfx.h"
#include "IGroundDropBatchRenderer.h"
#include "../Core/EventBus.h"
#include "../../EterBase/LogModern.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/StrongTypes.h"

#include <string_view>
#include <expected>
#include <mutex>
#include <unordered_map>
#include <memory>

namespace UserInterface::GroundDrop
{
    /**
     * @brief Zdarzenie oznaczajace usuniecie przedmiotu z ziemi.
     */
    struct GroundDropRemovedEvent : public Core::IEvent
    {
        EterBase::EntityId virtualId;

        /**
         * @brief Tworzy zdarzenie GroundDropRemovedEvent.
         * @param id Wirtualny identyfikator usunietego przedmiotu.
         * @return Oczekiwane zdarzenie lub blad.
         */
        static std::expected<GroundDropRemovedEvent, std::string_view> Create(EterBase::EntityId id)
        {
            if (id.value() == 0)
                return std::unexpected("Invalid virtualId");
            return GroundDropRemovedEvent(id);
        }

    private:
        explicit GroundDropRemovedEvent(EterBase::EntityId id) : virtualId(id) {}
    };

    class GroundDropBatchRendererRemoveImpl final : public IGroundDropBatchRenderer
    {
    public:
        EterBase::PacketResult<void> AddDrop(const GroundDropItemData& item) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_drops[item.virtualId] = item;
            return {};
        }

        EterBase::PacketResult<void> RemoveDrop(uint32_t virtualId) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            auto it = m_drops.find(virtualId);
            if (it == m_drops.end())
            {
                EterBase::ModernLogger::Error("RemoveDrop failed: virtualId {} not found.", virtualId);
                return EterBase::MakeError(EterBase::PacketError::SequenceMismatch);
            }

            m_drops.erase(it);
            EterBase::ModernLogger::Info("Removed drop virtualId {}.", virtualId);

            EterBase::EntityId eid(virtualId);
            auto eventResult = GroundDropRemovedEvent::Create(eid);
            
            if (eventResult.has_value())
            {
                Core::EventBus::GetInstance().Publish(eventResult.value());
            }
            else
            {
                EterBase::ModernLogger::Error("Failed to create GroundDropRemovedEvent: {}", eventResult.error());
            }

            return {};
        }

        void UpdateDrops(float /*deltaTime*/) override {}
        void RenderInstancedDrops() override {}
        bool IsInRangeToPick(uint32_t /*virtualId*/, float /*playerX*/, float /*playerY*/, float /*maxRange*/) const override
        {
            return false;
        }

        void ClearAll() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_drops.clear();
        }

    private:
        std::unordered_map<uint32_t, GroundDropItemData> m_drops;
        std::mutex m_mutex;
    };

    std::unique_ptr<IGroundDropBatchRenderer> CreateGroundDropBatchRendererRemoveImpl()
    {
        return std::make_unique<GroundDropBatchRendererRemoveImpl>();
    }
}
