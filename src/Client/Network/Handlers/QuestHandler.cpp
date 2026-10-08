#include "QuestHandler.h"

#ifndef TEST_COMPILATION
#include "../../../UserInterface/Packet.h"
#include "../../../UserInterface/PythonQuest.h"
#include "../../../UserInterface/PythonEventManager.h"
#include "../../../UserInterface/PythonNetworkStream.h"
#include "../../../UserInterface/Core/EventBus.h"
#include "../../../EterBase/LogModern.h"
#endif

#include <cstring>
#include <string>

namespace Network::Handlers
{
    namespace Events
    {
        struct QuestUpdatedEvent : public UserInterface::Core::IEvent {
            uint16_t index;
            explicit QuestUpdatedEvent(uint16_t idx) : index(idx) {}
        };
        struct ScriptStartedEvent : public UserInterface::Core::IEvent {
            uint8_t skin; int index;
            ScriptStartedEvent(uint8_t s, int i) : skin(s), index(i) {}
        };
        struct WarpEvent : public UserInterface::Core::IEvent {
            int32_t lX; int32_t lY;
            int32_t lAddr; uint16_t wPort;
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
        enum QuestPacketType { QUEST_PACKET_TYPE_NONE, QUEST_PACKET_TYPE_BEGIN, QUEST_PACKET_TYPE_UPDATE, QUEST_PACKET_TYPE_END };
        QuestPacketType packetType = QUEST_PACKET_TYPE_NONE;

        if ((flag & QUEST_SEND_IS_BEGIN) != 0) {
            uint8_t isBegin = 0;
            if (!SafeRead(isBegin)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            packetType = isBegin ? QUEST_PACKET_TYPE_BEGIN : QUEST_PACKET_TYPE_END;
        } else {
            packetType = QUEST_PACKET_TYPE_UPDATE;
        }

        char title[31] = {0}; char clockName[17] = {0}; int32_t clockValue = 0;
        char counterName[17] = {0}; int32_t counterValue = 0; char iconFileName[25] = {0};

        if ((flag & QUEST_SEND_TITLE) != 0 && !SafeReadString(title, 30)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if ((flag & QUEST_SEND_CLOCK_NAME) != 0 && !SafeReadString(clockName, 16)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if ((flag & QUEST_SEND_CLOCK_VALUE) != 0 && !SafeRead(clockValue)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if ((flag & QUEST_SEND_COUNTER_NAME) != 0 && !SafeReadString(counterName, 16)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if ((flag & QUEST_SEND_COUNTER_VALUE) != 0 && !SafeRead(counterValue)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        if ((flag & QUEST_SEND_ICON_FILE) != 0 && !SafeReadString(iconFileName, 24)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);

        CPythonQuest& pythonQuest = CPythonQuest::Instance();
        if (packetType == QUEST_PACKET_TYPE_END) {
            pythonQuest.DeleteQuestInstance(questInfo.index);
        } else if (packetType == QUEST_PACKET_TYPE_UPDATE) {
            if (!pythonQuest.IsQuest(questInfo.index)) pythonQuest.MakeQuest(questInfo.index);
            if (title[0] != '\0') pythonQuest.SetQuestTitle(questInfo.index, title);
            if (clockName[0] != '\0') pythonQuest.SetQuestClockName(questInfo.index, clockName);
            if (counterName[0] != '\0') pythonQuest.SetQuestCounterName(questInfo.index, counterName);
            if (iconFileName[0] != '\0') pythonQuest.SetQuestIconFileName(questInfo.index, iconFileName);
            if ((flag & QUEST_SEND_CLOCK_VALUE) != 0) pythonQuest.SetQuestClockValue(questInfo.index, clockValue);
            if ((flag & QUEST_SEND_COUNTER_VALUE) != 0) pythonQuest.SetQuestCounterValue(questInfo.index, counterValue);
        } else if (packetType == QUEST_PACKET_TYPE_BEGIN) {
            CPythonQuest::SQuestInstance q; q.dwIndex = questInfo.index;
            q.strTitle = title; q.strClockName = clockName; q.iClockValue = clockValue;
            q.strCounterName = counterName; q.iCounterValue = counterValue; q.strIconFileName = iconFileName;
            pythonQuest.RegisterQuestInstance(q);
        }

        UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestUpdatedEvent{questInfo.index});
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

        int iIndex = CPythonEventManager::Instance().RegisterEventSetFromString(str);
        if (-1 != iIndex) {
            CPythonEventManager::Instance().SetVisibleLineCount(iIndex, 30);
            CPythonNetworkStream::Instance().OnScriptEventStart(sp.skin, iIndex);
            UserInterface::Core::EventBus::GetInstance().Publish(Events::ScriptStartedEvent{sp.skin, iIndex});
        }
        return {};
    }

    EterBase::PacketResult<void> QuestHandler::HandleWarp(std::span<const uint8_t> buffer) {
        if (buffer.size() < sizeof(TPacketGCWarp)) return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        TPacketGCWarp w; std::memcpy(&w, buffer.data(), sizeof(TPacketGCWarp));
        UserInterface::Core::EventBus::GetInstance().Publish(Events::WarpEvent{w.lX, w.lY, w.lAddr, w.wPort});
        return {};
    }
}
