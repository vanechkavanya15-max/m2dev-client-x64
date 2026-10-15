#include "../StdAfx.h"
#include "../Packet.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

#include "IInventoryService.h"

#include "../Core/EventBus.h"
#include "EterBase/LogModern.h"

#include <memory>
#include <string_view>
#include <expected>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <any>

namespace UserInterface::Services
{
    /**
     * @brief Zdarzenie emitowane po udanej rejestracji serwisu.
     */
    struct ServiceRegisteredEvent : public Core::IEvent
    {
        std::string_view serviceName;

        explicit ServiceRegisteredEvent(std::string_view name) : serviceName(name) {}
    };

    /**
     * @brief Zdarzenie emitowane po wyczyszczeniu wszystkich serwisow.
     */
    struct ServicesClearedEvent : public Core::IEvent
    {
    };

    /**
     * @brief Singleton dostepu do rejestru (Service Locator) oparty o ukryta implementacje.
     */
    class PlayerServiceRegistryImpl
    {
    public:
        PlayerServiceRegistryImpl()
        {
            EterBase::ModernLogger::Info("PlayerServiceRegistry initialized.");
        }

        ~PlayerServiceRegistryImpl()
        {
            EterBase::ModernLogger::Info("PlayerServiceRegistry destroyed.");
        }

        void RegisterInventoryService(std::unique_ptr<IInventoryService> service)
        {
            inventoryService_ = std::move(service);
            EterBase::ModernLogger::Debug("IInventoryService registered in PlayerServiceRegistry.");
            Core::EventBus::GetInstance().Publish(ServiceRegisteredEvent{"IInventoryService"});
        }

        std::expected<std::reference_wrapper<IInventoryService>, EterBase::EntityError> GetInventoryService() const
        {
            if (!inventoryService_) {
                EterBase::ModernLogger::Error("IInventoryService not found in registry.");
                return std::unexpected(EterBase::EntityError::NotFound);
            }
            return std::ref(*inventoryService_);
        }

        void ClearAll()
        {
            if (inventoryService_) inventoryService_->Clear();
            
            EterBase::ModernLogger::Debug("All services cleared in PlayerServiceRegistry.");
            Core::EventBus::GetInstance().Publish(ServicesClearedEvent{});
        }

    private:
        std::unique_ptr<IInventoryService> inventoryService_;
    };
    
    // Z uwagi na zasade Zero-Conflict i brak istniejacego naglowka kontraktu dla rejestru,
    // API tego pliku zostalo przygotowane, jednak bez naglowka nikt z zewnatrz go nie uzyje w C++.
    
    static PlayerServiceRegistryImpl g_registry;
}
