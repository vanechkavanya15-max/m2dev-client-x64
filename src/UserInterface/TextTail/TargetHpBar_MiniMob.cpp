#include "../StdAfx.h"

#include "ITargetHpBarService.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"
#include "UserInterface/Core/EventBus.h"

#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <span>
#include <memory>

namespace UserInterface::TextTail
{
    /**
     * @brief Structure representing the health information of a target.
     */
    struct TargetHpInfo
    {
        uint32_t currentHp{0};
        uint32_t maxHp{0};
    };

    /**
     * @brief Raw packet struct for mini mob HP updates (simulated for parsing).
     */
#pragma pack(push, 1)
    struct TPacketGCMiniMobHp
    {
        uint8_t header;
        uint32_t vid;
        uint32_t currentHp;
        uint32_t maxHp;
    };
#pragma pack(pop)

    /**
     * @brief Implementation of ITargetHpBarService for handling mini mob HP bars.
     * 
     * Adheres to the Single Responsibility Principle, decoupling the GUI updates
     * using EventBus.
     */
    class TargetHpBarService_MiniMob final : public ITargetHpBarService
    {
    public:
        TargetHpBarService_MiniMob() = default;
        ~TargetHpBarService_MiniMob() override = default;

        /**
         * @brief Parses incoming packet data to update mini mob HP.
         * Fulfills C++23 requirement for PacketResult<void>.
         * @param buffer The binary packet data.
         * @return PacketResult<void> indicating success or underflow.
         */
        EterBase::PacketResult<void> HandleMiniMobHpPacket(std::span<const uint8_t> buffer)
        {
            if (buffer.size() < sizeof(TPacketGCMiniMobHp))
            {
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const TPacketGCMiniMobHp*>(buffer.data());
            
            EterBase::EntityId entityId(packet->vid);
            UpdateTargetHp(entityId, packet->currentHp, packet->maxHp);
            
            return {};
        }

        /**
         * @brief Shows or registers the target HP bar for a specific entity.
         * @param vid The unique ID of the entity.
         * @param currentHp The current HP of the entity.
         * @param maxHp The maximum HP of the entity.
         */
        void ShowTargetBar(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            if (!vid)
            {
                EterBase::ModernLogger::Warning("TargetHpBarService_MiniMob::ShowTargetBar - Invalid entity ID.");
                return;
            }

            std::lock_guard<std::mutex> lock(mutex_);
            hpMap_[vid] = TargetHpInfo{currentHp, maxHp};

            EterBase::ModernLogger::Debug("TargetHpBarService_MiniMob::ShowTargetBar - Registered HP for entity {}", vid.value());

            // Publish an event to decouple the GUI refresh
            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(vid.value()));
        }

        /**
         * @brief Updates the target HP for an existing entity.
         * @param vid The unique ID of the entity.
         * @param currentHp The updated current HP.
         * @param maxHp The updated max HP.
         */
        void UpdateTargetHp(EterBase::EntityId vid, uint32_t currentHp, uint32_t maxHp) override
        {
            if (!vid)
            {
                EterBase::ModernLogger::Warning("TargetHpBarService_MiniMob::UpdateTargetHp - Invalid entity ID.");
                return;
            }

            std::lock_guard<std::mutex> lock(mutex_);
            auto it = hpMap_.find(vid);
            if (it != hpMap_.end())
            {
                it->second.currentHp = currentHp;
                it->second.maxHp = maxHp;
            }
            else
            {
                hpMap_[vid] = TargetHpInfo{currentHp, maxHp};
            }

            EterBase::ModernLogger::Debug("TargetHpBarService_MiniMob::UpdateTargetHp - Updated HP for entity {}", vid.value());

            // Publish an event to notify systems of the target HP update
            Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(vid.value()));
        }

        /**
         * @brief Hides the main target bar (stubbed for mini mob logic).
         */
        void HideTargetBar() override
        {
            EterBase::ModernLogger::Debug("TargetHpBarService_MiniMob::HideTargetBar - Hiding target bar (stub)");
        }

        /**
         * @brief Renders the main target bar (stubbed for mini mob logic).
         */
        void RenderTargetBar() override
        {
            // Only stubbed in this implementation, GUI systems could intercept this via events 
            // if needed, or it might be managed via independent Python UI queries.
            // EterBase::ModernLogger::Trace("TargetHpBarService_MiniMob::RenderTargetBar - Invoked");
        }

        /**
         * @brief Iterates over tracked mobs and performs rendering operations.
         */
        void RenderMiniMobBars() override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [vid, hpInfo] : hpMap_)
            {
                if (hpInfo.currentHp > 0)
                {
                    // Trigger a refresh event for the GUI to render the mini HP bar for this specific mob.
                    // This strictly decouples C++ state from GUI rendering logic (CPythonTextTail).
                    Core::EventBus::GetInstance().Publish(Core::TargetBoardRefreshEvent(vid.value()));
                }
            }
        }

        /**
         * @brief Clears all tracked target HP data.
         */
        void Clear() override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            hpMap_.clear();
            EterBase::ModernLogger::Info("TargetHpBarService_MiniMob::Clear - Cleared all mini mob HP bars");
        }

    private:
        std::mutex mutex_;
        std::unordered_map<EterBase::EntityId, TargetHpInfo> hpMap_;
    };

    /**
     * @brief Factory function to create an instance of ITargetHpBarService for mini mobs.
     * @return A unique pointer to the newly created service.
     */
    std::unique_ptr<ITargetHpBarService> CreateTargetHpBarService_MiniMob()
    {
        return std::make_unique<TargetHpBarService_MiniMob>();
    }
}
