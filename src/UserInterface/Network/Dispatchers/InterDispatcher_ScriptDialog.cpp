#include "../../StdAfx.h"
#include "../PacketDispatcher.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../Packet.h"

#include <span>
#include <cstdint>
#include <cstring>
#include <string>

namespace Network::Dispatchers
{
    /**
     * @brief Zdarzenie dialogu skryptowego emitowane do warstwy prezentacji.
     */
    struct ScriptDialogEvent : public UserInterface::Core::IEvent
    {
        uint8_t skin;
        std::string scriptData;

        ScriptDialogEvent(uint8_t s, std::string data)
            : skin(s), scriptData(std::move(data))
        {
        }
    };

    /**
     * @brief Przetwarza pakiet dialogu skryptowego i publikuje zdarzenie do GUI.
     * @param buffer Bufor bajtow zawierajacy caly pakiet (lacznie ze zmienna dlugoscia payloadu).
     * @return EterBase::PacketResult<void> ze scislym wynikiem operacji (sukces lub kod bledu).
     */
    EterBase::PacketResult<void> ProcessScriptDialog(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCScript))
        {
            EterBase::ModernLogger::Error("ProcessScriptDialog: Buffer too small for header ({} < {})",
                                          buffer.size(), sizeof(TPacketGCScript));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCScript packet{};
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCScript));

        if (buffer.size() < packet.length)
        {
            EterBase::ModernLogger::Error("ProcessScriptDialog: Buffer too small for payload ({} < {})",
                                          buffer.size(), packet.length);
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        if (packet.length < sizeof(TPacketGCScript))
        {
            EterBase::ModernLogger::Error("ProcessScriptDialog: Invalid packet length ({} < {})",
                                          packet.length, sizeof(TPacketGCScript));
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        const size_t payloadSize = packet.length - sizeof(TPacketGCScript);
        std::string scriptText;
        
        if (payloadSize > 0)
        {
            // Odbior tekstu z bufora bez uzycia reinterpret_cast
            scriptText.assign(buffer.begin() + sizeof(TPacketGCScript), buffer.begin() + sizeof(TPacketGCScript) + payloadSize);
            
            // Jesli string mial na koncu Null-Terminator z serwera, usuwamy go ze stringa
            if (!scriptText.empty() && scriptText.back() == '\0')
            {
                scriptText.pop_back();
            }
        }

        EterBase::ModernLogger::Info("ProcessScriptDialog: Received script dialog packet (Skin: {}, Size: {})",
                                     packet.skin, payloadSize);

        ScriptDialogEvent event(packet.skin, std::move(scriptText));
        UserInterface::Core::EventBus::GetInstance().Publish(event);

        return {};
    }

    /**
     * @brief Rejestrator Dispatchera dla GC::SCRIPT (0x0910).
     */
    void RegisterScriptDialogDispatcher()
    {
        constexpr uint16_t HEADER_GC_SCRIPT = 0x0910;
        Network::PacketDispatcher::Instance().RegisterModernHandler(
            HEADER_GC_SCRIPT,
            ProcessScriptDialog
        );
    }
}
