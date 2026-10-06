#include "../StdAfx.h"
#include "ITextTailService.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include "../PythonTextTail.h"

// Deklaracje powiazanych zdarzen w celu zapewnienia odpowiedniego dopasowania typu przez EventBus.
namespace Network::Handlers
{
    struct PhaseChangedEvent : public UserInterface::Core::IEvent
    {
        uint8_t phase;
        explicit PhaseChangedEvent(uint8_t phase) : phase(phase) {}
    };
}

namespace Network::Dispatchers
{
    struct WarpEvent : public UserInterface::Core::IEvent
    {
        int32_t x;
        int32_t y;
        int32_t ipAddress;
        uint16_t port;
        
        WarpEvent(int32_t x, int32_t y, int32_t ipAddress, uint16_t port)
            : x(x), y(y), ipAddress(ipAddress), port(port) {}
    };
}

namespace UserInterface::TextTail
{
    namespace
    {
        /**
         * @brief Automatyczny rejestrator zdarzen dla TextTail.
         * 
         * Subskrybuje zdarzenia zmiany mapy i teleportacji, aby wyczyscic wszystkie
         * etykiety tekstowe z ekranu zgodnie z zasada Zero-Conflict.
         */
        struct ClearAllRegistration
        {
            ClearAllRegistration()
            {
                auto& eventBus = Core::EventBus::GetInstance();

                eventBus.Subscribe<Network::Handlers::PhaseChangedEvent>([](const Network::Handlers::PhaseChangedEvent& event) {
                    EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "Phase change detected (Phase: {}). Clearing all TextTails.", event.phase);
                    CPythonTextTail::Instance().Clear();
                });

                eventBus.Subscribe<Network::Dispatchers::WarpEvent>([](const Network::Dispatchers::WarpEvent& event) {
                    EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "Warp detected (X: {}, Y: {}). Clearing all TextTails.", event.x, event.y);
                    CPythonTextTail::Instance().Clear();
                });
            }
        } g_clearAllRegistration;
    }
}
