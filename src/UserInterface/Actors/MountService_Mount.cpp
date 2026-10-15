#include "../StdAfx.h"
#include "IMountHorseService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"

#include <unordered_map>
#include <mutex>
#include <memory>
#include <expected>
#include <string_view>

namespace UserInterface::Actors
{
    class MountHorseService : public IMountHorseService
    {
    public:
        MountHorseService() = default;
        ~MountHorseService() override = default;

        void Mount(EterBase::EntityId riderId, uint32_t mountVnum) override
        {
            EterBase::ItemVnum vnum(mountVnum);
            
            auto validationResult = ValidateMount(riderId, vnum);
            if (!validationResult.has_value())
            {
                EterBase::ModernLogger::Error("MountService: Failed to mount rider {}. Error: {}", 
                    riderId.get(), EterBase::ToString(validationResult.error()));
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_mounts[riderId] = vnum.get();
            }

            EterBase::ModernLogger::Info("MountService: Rider {} mounted on {}.", riderId.get(), vnum.get());

            UserInterface::Core::MountStateChangedEvent event(riderId.get(), vnum.get(), 1);
            UserInterface::Core::EventBus::GetInstance().Publish(event);
        }

        void Dismount(EterBase::EntityId riderId) override
        {
            if (!riderId)
            {
                EterBase::ModernLogger::Error("MountService: Failed to dismount. Invalid rider ID.");
                return;
            }

            EterBase::ItemVnum mountVnum(0);
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_mounts.find(riderId);
                if (it == m_mounts.end())
                {
                    EterBase::ModernLogger::Warning("MountService: Rider {} is not mounted.", riderId.get());
                    return;
                }
                mountVnum = EterBase::ItemVnum(it->second);
                m_mounts.erase(it);
            }

            EterBase::ModernLogger::Info("MountService: Rider {} dismounted from {}.", riderId.get(), mountVnum.get());

            UserInterface::Core::MountStateChangedEvent event(riderId.get(), 0, 0);
            UserInterface::Core::EventBus::GetInstance().Publish(event);
        }

        bool IsMounted(EterBase::EntityId riderId) const override
        {
            if (!riderId) return false;

            std::lock_guard<std::mutex> lock(m_mutex);
            return m_mounts.find(riderId) != m_mounts.end();
        }

        uint32_t GetMountVnum(EterBase::EntityId riderId) const override
        {
            if (!riderId) return 0;

            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_mounts.find(riderId);
            return it != m_mounts.end() ? it->second : 0;
        }

        void Clear() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_mounts.clear();
            EterBase::ModernLogger::Info("MountService: Cleared all mounts.");
        }

    private:
        std::expected<void, EterBase::EntityError> ValidateMount(EterBase::EntityId riderId, EterBase::ItemVnum mountVnum) const
        {
            if (!riderId)
            {
                return std::unexpected(EterBase::EntityError::NotFound);
            }
            if (!mountVnum)
            {
                return std::unexpected(EterBase::EntityError::InvalidType);
            }
            return {};
        }

        mutable std::mutex m_mutex;
        std::unordered_map<EterBase::EntityId, uint32_t> m_mounts;
    };

    std::unique_ptr<IMountHorseService> CreateMountHorseService()
    {
        return std::make_unique<MountHorseService>();
    }
}
