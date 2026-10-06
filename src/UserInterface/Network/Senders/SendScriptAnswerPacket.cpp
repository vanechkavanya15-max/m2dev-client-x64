#include "../../StdAfx.h"

/**
 * @file SendScriptAnswerPacket.cpp
 * @brief Wysłanie odpowiedzi w oknie questa NPC (Modernizacja C++23).
 *
 * Plik utworzony zgodnie z architekturą Zero-Conflict Standard 2026.
 * Dekapsulacja od CPythonNetworkStream (event-driven C++23).
 */

#include "../../Packet.h"
#include "../../PythonNetworkStream.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../Core/EventBus.h"

#include <optional>

namespace Network::Senders {

    /**
     * @brief Zdarzenie emitowane po pomyślnym wysłaniu odpowiedzi do skryptu NPC.
     */
    struct ScriptAnswerSentEvent : public UserInterface::Core::IEvent {
        uint8_t answer;

        /**
         * @brief Konstruktor zdarzenia.
         * @param answer Wybrany indeks odpowiedzi w dialogu.
         */
        explicit ScriptAnswerSentEvent(uint8_t answer) : answer(answer) {}
    };

    /**
     * @brief Formatuje i wysyła pakiet odpowiedzi questa NPC.
     * 
     * @param stream Opcjonalny wskaźnik do instancji CPythonNetworkStream odpowiedzialnej za wysyłanie.
     * @param answer Wybrany indeks odpowiedzi w dialogu.
     * @return EterBase::PacketResult<void> Sukces (void) lub PacketError przy braku streamu / błędzie wysyłania.
     */
    EterBase::PacketResult<void> SendScriptAnswer(std::optional<CPythonNetworkStream*> stream, uint8_t answer)
    {
        return stream
            .and_then([](CPythonNetworkStream* s) -> std::optional<CPythonNetworkStream*> {
                if (s != nullptr) return s;
                return std::nullopt;
            })
            .transform([answer](CPythonNetworkStream* validStream) -> EterBase::PacketResult<void> {
                TPacketCGScriptAnswer packet{};
                packet.header = CG::SCRIPT_ANSWER;
                packet.length = sizeof(packet);
                packet.answer = answer;

                if (!validStream->Send(sizeof(packet), &packet))
                {
                    EterBase::ModernLogger::Error("SendScriptAnswer failed: Stream Send returned false.");
                    return EterBase::MakeError(EterBase::PacketError::SessionClosed);
                }

                EterBase::ModernLogger::Info("SendScriptAnswerPacket sent successfully (answer: {})", answer);
                
                // Emitowanie zdarzenia o wysłaniu odpowiedzi do szyny zdarzeń
                UserInterface::Core::EventBus::GetInstance().Publish(ScriptAnswerSentEvent{answer});
                
                return {};
            })
            .value_or(
                []() -> EterBase::PacketResult<void> {
                    EterBase::ModernLogger::Error("SendScriptAnswer failed: Network stream is null.");
                    return EterBase::MakeError(EterBase::PacketError::SessionClosed);
                }()
            );
    }

} // namespace Network::Senders
