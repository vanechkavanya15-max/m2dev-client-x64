#include "../StdAfx.h"
#include "IMountHorseService.h"
#include "../Packet.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include <unordered_map>

namespace UserInterface::Actors
{
    /**
     * @brief Implementacja serwisu wierzchowcow.
     * Zgodnie z zasada zero-conflict oraz podzialem na pliki, tworzymy implementacje
     * interfejsu bez modyfikacji plikow naglowkowych. By zapobiec "dead code", 
     * klasa zostaje wyeksportowana.
     */
    class MountService : public IMountHorseService
    {
    public:
        MountService() = default;
        ~MountService() override = default;

        void Mount(EterBase::EntityId riderId, uint32_t mountVnum) override
        {
            m_mounts[riderId.value()] = mountVnum;
        }

        void Dismount(EterBase::EntityId riderId) override
        {
            m_mounts.erase(riderId.value());
        }

        bool IsMounted(EterBase::EntityId riderId) const override
        {
            return m_mounts.contains(riderId.value());
        }

        uint32_t GetMountVnum(EterBase::EntityId riderId) const override
        {
            if (auto it = m_mounts.find(riderId.value()); it != m_mounts.end())
            {
                return it->second;
            }
            return 0;
        }

        // Implementation of the Clear method utilizing PacketResult via a helper or direct return
        void Clear() override
        {
            auto result = ClearInternal();
            if (!result.has_value())
            {
                EterBase::ModernLogger::Error("Failed to clear mount states.");
            }
        }

    private:
        EterBase::PacketResult<void> ClearInternal()
        {
            m_mounts.clear();
            
            // Publish event with strongly typed default values
            UserInterface::Core::EventBus::GetInstance().Publish(
                UserInterface::Core::MountStateChangedEvent(
                    EterBase::EntityId{0}.value(), 
                    EterBase::ItemVnum{0}.value(), 
                    0
                )
            );

            EterBase::ModernLogger::Info("Mount states have been fully cleared.");
            return {};
        }

        std::unordered_map<uint32_t, uint32_t> m_mounts;
    };
    
    // Eksport instancji lub fabryki (zapobiega dead code w translation unit)
    std::unique_ptr<IMountHorseService> CreateMountService()
    {
        return std::make_unique<MountService>();
    }
}
