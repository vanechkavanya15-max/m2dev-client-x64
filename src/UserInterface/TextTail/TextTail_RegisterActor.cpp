#include "../StdAfx.h"
#include "TextTailService.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"
#include <string>

namespace UserInterface::TextTail
{
    namespace
    {
        /**
         * @brief Zdarzenie rozglaszane po pomyslnym zarejestrowaniu etykiety (text tail) dla aktora.
         */
        struct ActorTextTailRegisteredEvent : public Core::IEvent
        {
            EterBase::EntityId vid;
            std::string text;
            uint32_t color;
            float offsetY;

            ActorTextTailRegisteredEvent(EterBase::EntityId v, std::string_view t, uint32_t c, float o)
                : vid(v), text(std::string(t)), color(c), offsetY(o) {}
        };
    } // namespace anonymous

    /**
     * @brief Zabezpieczona implementacja rejestracji etykiety aktora uzywajaca nowoczesnych wzorcow C++23.
     */
    EterBase::PacketResult<void> TextTailService::RegisterActorTail(const TextTailCreateData& data)
    {
        if (!data.vid)
        {
            EterBase::ModernLogger::Error("TextTailService::RegisterActorTail - Brak identyfikatora VID!");
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        EterBase::ModernLogger::Info(
            "TextTailService::RegisterActorTail - Pomyslnie zarejestrowano. VID: {}, Kolor: {:#x}, OffsetY: {}",
            data.vid.value(), data.color, data.offsetY
        );

        Core::EventBus::GetInstance().Publish(ActorTextTailRegisteredEvent{
            data.vid, data.text, data.color, data.offsetY
        });

        return {};
    }

} // namespace UserInterface::TextTail
