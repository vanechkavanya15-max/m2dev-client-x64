#include "QuestDialogDomainHandler.h"
#include "EterBase/LogModern.h"
#include <cstring>
#include <algorithm>

namespace Client::Network::Handlers {

EterBase::PacketResult<void> QuestDialogDomainHandler::HandleScriptPacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCScript)) {
        EterBase::ModernLogger::Error("QuestDialogDomainHandler: Bufor TPacketGCScript za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCScript));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* header = reinterpret_cast<const TPacketGCScript*>(payload.data());
    if (header->length < sizeof(TPacketGCScript)) {
        EterBase::ModernLogger::Error("QuestDialogDomainHandler: Nieprawidlowy rozmiar skryptu w naglowku: {}", header->length);
        return std::unexpected(EterBase::PacketError::MalformedPayload);
    }

    size_t scriptSize = header->length - sizeof(TPacketGCScript);
    if (payload.size() < sizeof(TPacketGCScript) + scriptSize) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const char* scriptData = reinterpret_cast<const char*>(payload.data() + sizeof(TPacketGCScript));
    std::string scriptStr(scriptData, scriptSize);

    EterBase::ModernLogger::Info("QuestDialogDomainHandler: Odebrano skrypt dialogowy questa, dlugosc: {}, skin: {}",
        scriptSize, header->skin);

    Client::Core::Events::QuestScriptDialogOpenedEvent event(header->skin, std::move(scriptStr));
    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

EterBase::PacketResult<void> QuestDialogDomainHandler::HandleQuestInfoPacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCQuestInfo)) {
        EterBase::ModernLogger::Error("QuestDialogDomainHandler: Bufor TPacketGCQuestInfo za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCQuestInfo));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* info = reinterpret_cast<const TPacketGCQuestInfo*>(payload.data());
    if (payload.size() < info->length) {
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    Client::Core::Events::QuestInfoReceivedEvent event{};
    event.flag = info->flag;

    size_t offset = sizeof(TPacketGCQuestInfo);
    if (offset < payload.size()) {
        const char* strData = reinterpret_cast<const char*>(payload.data() + offset);
        event.title = std::string(strData, strnlen(strData, payload.size() - offset));
    }

    EterBase::ModernLogger::Debug("QuestDialogDomainHandler: Odebrano aktualizacje misji: flag 0x{:08X}", info->flag);
    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

EterBase::PacketResult<void> QuestDialogDomainHandler::HandleQuestConfirmPacket(std::span<const uint8_t> payload) {
    if (payload.size() < sizeof(TPacketGCQuestConfirm)) {
        EterBase::ModernLogger::Error("QuestDialogDomainHandler: Bufor TPacketGCQuestConfirm za krotki ({} < {})",
            payload.size(), sizeof(TPacketGCQuestConfirm));
        return std::unexpected(EterBase::PacketError::BufferUnderflow);
    }

    const auto* confirm = reinterpret_cast<const TPacketGCQuestConfirm*>(payload.data());
    std::string msg(confirm->msg, strnlen(confirm->msg, sizeof(confirm->msg)));

    EterBase::ModernLogger::Info("QuestDialogDomainHandler: Zadanie potwierdzenia: '{}', timeout: {}s, PID: {}",
        msg, confirm->timeout, confirm->requestPID);

    Client::Core::Events::QuestConfirmRequestedEvent event(std::move(msg), confirm->timeout, confirm->requestPID);
    Client::Core::EventBus::GetInstance().Publish(event);

    return {};
}

std::vector<uint8_t> QuestDialogDomainHandler::EncodeScriptAnswer(int32_t answer) {
    TPacketCGScriptAnswer packet{};
    packet.header = CG::SCRIPT_ANSWER;
    packet.length = sizeof(TPacketCGScriptAnswer);
    packet.answer = static_cast<uint8_t>(answer);

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> QuestDialogDomainHandler::EncodeScriptButton(uint32_t index) {
    TPacketCGScriptButton packet{};
    packet.header = CG::SCRIPT_BUTTON;
    packet.length = sizeof(TPacketCGScriptButton);
    packet.idx = index;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> QuestDialogDomainHandler::EncodeScriptSelectItem(uint32_t itemPos) {
    TPacketCGScriptSelectItem packet{};
    packet.header = CG::SCRIPT_SELECT_ITEM;
    packet.length = sizeof(TPacketCGScriptSelectItem);
    packet.selection = itemPos;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> QuestDialogDomainHandler::EncodeQuestInputString(std::string_view input) {
    TPacketCGQuestInputString packet{};
    packet.header = CG::QUEST_INPUT_STRING;
    packet.length = sizeof(TPacketCGQuestInputString);
    std::memcpy(packet.szString, input.data(), std::min(input.size(), static_cast<size_t>(QUEST_INPUT_STRING_MAX_NUM)));

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> QuestDialogDomainHandler::EncodeQuestConfirm(uint8_t answer, uint32_t requestPid) {
    TPacketCGQuestConfirm packet{};
    packet.header = CG::QUEST_CONFIRM;
    packet.length = sizeof(TPacketCGQuestConfirm);
    packet.answer = answer;
    packet.requestPID = requestPid;

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

std::vector<uint8_t> QuestDialogDomainHandler::EncodeQuestCancel() {
    TPacketCGQuestCancel packet{};
    packet.header = CG::QUEST_CANCEL;
    packet.length = sizeof(TPacketCGQuestCancel);

    std::vector<uint8_t> buffer(sizeof(packet));
    std::memcpy(buffer.data(), &packet, sizeof(packet));
    return buffer;
}

} // namespace Client::Network::Handlers
