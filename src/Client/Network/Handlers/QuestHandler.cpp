#include "QuestHandler.h"

#include "../Protocol/Protocol.h"
#include "Client/Core/EventBus.h"
#include "../../../EterBase/LogModern.h"

#include <cstring>
#include <string>

namespace Network::Handlers
{
    namespace Events
    {
        enum class QuestPacketType : uint8_t { None, Begin, Update, End };

        struct QuestUpdatedEvent : public Client::Core::IEvent {
            uint16_t index{0};
            QuestPacketType packetType{QuestPacketType::None};
            std::string title;
            std::string clockName;
            int32_t clockValue{0};
            std::string counterName;
            int32_t counterValue{0};
            std::string iconFileName;

            QuestUpdatedEvent(uint16_t idx, QuestPacketType type, std::string t = "", std::string clk = "", int32_t clkVal = 0,
                              std::string cnt = "", int32_t cntVal = 0, std::string icon = "")
                : index(idx), packetType(type), title(std::move(t)), clockName(std::move(clk)), clockValue(clkVal),
                  counterName(std::move(cnt)), counterValue(cntVal), iconFileName(std::move(icon)) {}
        };

        struct ScriptStartedEvent : public Client::Core::IEvent {
            uint8_t skin{0};
            std::string script;

            ScriptStartedEvent(uint8_t s, std::string scr)
                : skin(s), script(std::move(scr)) {}
        };

        struct WarpEvent : public Client::Core::IEvent {
            int32_t lX{0};
            int32_t lY{0};
            int32_t lAddr{0};
            uint16_t wPort{0};
            WarpEvent(int32_t x, int32_t y, int32_t addr, uint16_t port) : lX(x), lY(y), lAddr(addr), wPort(port) {}
        };
    }

    EterBase::PacketResult<void> QuestHandler::HandleQuestInfo(std::span<const uint8_t> buffer) {
        size_t offset = 0;
        auto SafeRead = [&]<typename T>(T& out) -> bool {
            if (offset + sizeof(T) > buffer.size()) return false;
            std::memcpy(&out, buffer.data() + offset, sizeof(T));
            offset += sizeof(T);
            return true;
        };
        auto SafeReadString = [&](char* out, size_t maxLength) -> bool {
            if (offset + maxLength > buffer.size()) return false;
            std::memcpy(out, buffer.data() + offset, maxLength);
            out[maxLength] = '\0';
            offset += maxLength;
            return true;
        };

        TPacketGCQuestInfo questInfo;
        if (!SafeRead(questInfo)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        const uint8_t flag = questInfo.flag;
        Events::QuestPacketType packetType = Events::QuestPacketType::None;

        if ((flag & QUEST_SEND_IS_BEGIN) != 0) {
            uint8_t isBegin = 0;
            if (!SafeRead(isBegin)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            packetType = isBegin ? Events::QuestPacketType::Begin : Events::QuestPacketType::End;
        } else {
            packetType = Events::QuestPacketType::Update;
        }

        char title[31] = {0}; char clockName[17] = {0}; int32_t clockValue = 0;
        char counterName[17] = {0}; int32_t counterValue = 0; char iconFileName[25] = {0};

        if ((flag & QUEST_SEND_TITLE) != 0 && !SafeReadString(title, 30)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if ((flag & QUEST_SEND_CLOCK_NAME) != 0 && !SafeReadString(clockName, 16)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if ((flag & QUEST_SEND_CLOCK_VALUE) != 0 && !SafeRead(clockValue)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if ((flag & QUEST_SEND_COUNTER_NAME) != 0 && !SafeReadString(counterName, 16)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if ((flag & QUEST_SEND_COUNTER_VALUE) != 0 && !SafeRead(counterValue)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if ((flag & QUEST_SEND_ICON_FILE) != 0 && !SafeReadString(iconFileName, 24)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        Client::Core::EventBus::GetInstance().Publish(Events::QuestUpdatedEvent{
            questInfo.index, packetType, title, clockName, clockValue, counterName, counterValue, iconFileName
        });
        return {};
    }

    EterBase::PacketResult<void> QuestHandler::HandleScript(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCScript)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        TPacketGCScript sp; std::memcpy(&sp, buffer.data(), sizeof(TPacketGCScript));
        if (buffer.size() < sp.length) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if (sp.length < sizeof(TPacketGCScript)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        size_t strLen = sp.length - sizeof(TPacketGCScript);
        std::string str(reinterpret_cast<const char*>(buffer.data() + sizeof(TPacketGCScript)), strLen);
        if (!str.empty() && str.back() != '\0') str.push_back('\0');

        Client::Core::EventBus::GetInstance().Publish(Events::ScriptStartedEvent{sp.skin, std::move(str)});
        return {};
    }

    EterBase::PacketResult<void> QuestHandler::HandleWarp(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCWarp)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        TPacketGCWarp w; std::memcpy(&w, buffer.data(), sizeof(TPacketGCWarp));
        Client::Core::EventBus::GetInstance().Publish(Events::WarpEvent{w.lX, w.lY, w.lAddr, w.wPort});
        return {};
    }
}
