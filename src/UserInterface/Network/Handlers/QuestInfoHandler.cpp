#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../PythonQuest.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"

#include <cstdint>
#include <span>
#include <string_view>
#include <algorithm>
#include <cstring>
#include <format>

namespace Network::Handlers
{
    /**
     * @brief Zdarzenie rozglaszane przez EventBus po pomyslnej aktualizacji danych questu.
     * Umozliwia odpiecie warstwy GUI (Python) od bezposrednich powiazan w Handlerze.
     */
    struct QuestUpdatedEvent : public UserInterface::Core::IEvent
    {
        uint16_t index;

        explicit QuestUpdatedEvent(uint16_t idx) : index(idx) {}
    };

    /**
     * @brief Przetwarza pakiet HEADER_GC_QUEST_INFO i aktualizuje stan questu.
     * @param buffer Bufor pakietu przychodzacego.
     * @return EterBase::PacketResult<void> oznaczajacy poprawnosc (lub blad wczytywania).
     */
    EterBase::PacketResult<void> ProcessQuestInfoPacket(std::span<const uint8_t> buffer)
    {
        size_t offset = 0;

        auto SafeRead = [&]<typename T>(T& out) -> bool {
            if (offset + sizeof(T) > buffer.size())
                return false;
            std::memcpy(&out, buffer.data() + offset, sizeof(T));
            offset += sizeof(T);
            return true;
        };

        auto SafeReadString = [&](char* out, size_t maxLength) -> bool {
            if (offset + maxLength > buffer.size())
                return false;
            std::memcpy(out, buffer.data() + offset, maxLength);
            out[maxLength] = '\0';
            offset += maxLength;
            return true;
        };

        TPacketGCQuestInfo questInfo;
        if (!SafeRead(questInfo))
        {
            EterBase::ModernLogger::Error("ProcessQuestInfoPacket: Buffer underflow reading TPacketGCQuestInfo.");
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const uint8_t flag = questInfo.flag;

        enum QuestPacketType {
            QUEST_PACKET_TYPE_NONE,
            QUEST_PACKET_TYPE_BEGIN,
            QUEST_PACKET_TYPE_UPDATE,
            QUEST_PACKET_TYPE_END,
        };

        QuestPacketType packetType = QUEST_PACKET_TYPE_NONE;

        if ((flag & QUEST_SEND_IS_BEGIN) != 0)
        {
            uint8_t isBegin = 0;
            if (!SafeRead(isBegin))
            {
                EterBase::ModernLogger::Error("ProcessQuestInfoPacket: Buffer underflow reading isBegin.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            packetType = isBegin ? QUEST_PACKET_TYPE_BEGIN : QUEST_PACKET_TYPE_END;
        }
        else
        {
            packetType = QUEST_PACKET_TYPE_UPDATE;
        }

        char title[30 + 1] = { 0 };
        char clockName[16 + 1] = { 0 };
        int32_t clockValue = 0;
        char counterName[16 + 1] = { 0 };
        int32_t counterValue = 0;
        char iconFileName[24 + 1] = { 0 };

        if ((flag & QUEST_SEND_TITLE) != 0)
        {
            if (!SafeReadString(title, 30))
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }
        
        if ((flag & QUEST_SEND_CLOCK_NAME) != 0)
        {
            if (!SafeReadString(clockName, 16))
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        if ((flag & QUEST_SEND_CLOCK_VALUE) != 0)
        {
            if (!SafeRead(clockValue))
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        if ((flag & QUEST_SEND_COUNTER_NAME) != 0)
        {
            if (!SafeReadString(counterName, 16))
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        if ((flag & QUEST_SEND_COUNTER_VALUE) != 0)
        {
            if (!SafeRead(counterValue))
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        if ((flag & QUEST_SEND_ICON_FILE) != 0)
        {
            if (!SafeReadString(iconFileName, 24))
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        CPythonQuest& pythonQuest = CPythonQuest::Instance();

        if (packetType == QUEST_PACKET_TYPE_END)
        {
            pythonQuest.DeleteQuestInstance(questInfo.index);
        }
        else if (packetType == QUEST_PACKET_TYPE_UPDATE)
        {
            if (!pythonQuest.IsQuest(questInfo.index))
            {
                pythonQuest.MakeQuest(questInfo.index);
            }

            if (title[0] != '\0')
                pythonQuest.SetQuestTitle(questInfo.index, title);
            if (clockName[0] != '\0')
                pythonQuest.SetQuestClockName(questInfo.index, clockName);
            if (counterName[0] != '\0')
                pythonQuest.SetQuestCounterName(questInfo.index, counterName);
            if (iconFileName[0] != '\0')
                pythonQuest.SetQuestIconFileName(questInfo.index, iconFileName);

            if ((flag & QUEST_SEND_CLOCK_VALUE) != 0)
                pythonQuest.SetQuestClockValue(questInfo.index, clockValue);
            if ((flag & QUEST_SEND_COUNTER_VALUE) != 0)
                pythonQuest.SetQuestCounterValue(questInfo.index, counterValue);
        }
        else if (packetType == QUEST_PACKET_TYPE_BEGIN)
        {
            CPythonQuest::SQuestInstance questInstance;
            questInstance.dwIndex = questInfo.index;
            questInstance.strTitle = title;
            questInstance.strClockName = clockName;
            questInstance.iClockValue = clockValue;
            questInstance.strCounterName = counterName;
            questInstance.iCounterValue = counterValue;
            questInstance.strIconFileName = iconFileName;
            
            pythonQuest.RegisterQuestInstance(questInstance);
        }

        // Publikacja zdarzenia - odpiecie od interfejsu Pythona
        UserInterface::Core::EventBus::GetInstance().Publish(QuestUpdatedEvent{questInfo.index});

        return {};
    }

    /**
     * @brief Handler owijajacy funkcjonalnosc dla dispatchera kompatybilnosci.
     * @param buffer Bufor pakietu przychodzacego.
     * @return true jezeli przetworzenie sie powiodlo.
     */
    bool HandleQuestInfoPacket(std::span<const uint8_t> buffer)
    {
        return ProcessQuestInfoPacket(buffer).has_value();
    }
}
