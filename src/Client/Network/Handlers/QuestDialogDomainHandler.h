#pragma once

#include <span>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "EterBase/PacketResult.h"
#include "Client/Core/EventBus.h"
#include "Client/Network/Protocol/Protocol.h"

namespace Client::Core::Events {

/**
 * @brief Zdarzenie otwarcia okna dialogu zadania/skryptu NPC.
 */
struct QuestScriptDialogOpenedEvent : public Client::Core::IEvent {
    uint8_t skin{0};
    std::string script;

    QuestScriptDialogOpenedEvent(uint8_t s, std::string sc)
        : skin(s), script(std::move(sc)) {}
};

/**
 * @brief Zdarzenie aktualizacji danych zadania / misji (Quest Info).
 */
struct QuestInfoReceivedEvent : public Client::Core::IEvent {
    uint32_t flag{0};
    std::string title;
    std::string clockName;
    int32_t clockValue{0};
    std::string counterName;
    int32_t counterValue{0};
    std::string iconFileName;
};

/**
 * @brief Zdarzenie zadania potwierdzenia questa (okno z limitem czasu Tak/Nie).
 */
struct QuestConfirmRequestedEvent : public Client::Core::IEvent {
    std::string message;
    uint32_t timeoutSec{0};
    uint32_t requestPid{0};

    QuestConfirmRequestedEvent(std::string msg, uint32_t to, uint32_t pid)
        : message(std::move(msg)), timeoutSec(to), requestPid(pid) {}
};

} // namespace Client::Core::Events

namespace Client::Network::Handlers {

/**
 * @brief Zwarty handler domenowy SRP dla dialogow NPC i zadan (Quest).
 * Odpowiada za obsluge skryptow dialogowych, wyborow, przyciskow oraz wejscia tekstowego.
 */
class QuestDialogDomainHandler {
public:
    QuestDialogDomainHandler() = default;
    ~QuestDialogDomainHandler() = default;

    // Handlery wejsciowe (Inbound span decoders)
    static EterBase::PacketResult<void> HandleScriptPacket(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleQuestInfoPacket(std::span<const uint8_t> payload);
    static EterBase::PacketResult<void> HandleQuestConfirmPacket(std::span<const uint8_t> payload);

    // Koder pakietow wychodzacych (Outbound encoders)
    static std::vector<uint8_t> EncodeScriptAnswer(int32_t answer);
    static std::vector<uint8_t> EncodeScriptButton(uint32_t index);
    static std::vector<uint8_t> EncodeScriptSelectItem(uint32_t itemPos);
    static std::vector<uint8_t> EncodeQuestInputString(std::string_view input);
    static std::vector<uint8_t> EncodeQuestConfirm(uint8_t answer, uint32_t requestPid);
    static std::vector<uint8_t> EncodeQuestCancel();
};

} // namespace Client::Network::Handlers
