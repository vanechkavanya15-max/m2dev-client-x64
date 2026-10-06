#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <cstring>
#include <optional>

namespace Network::Dispatchers
{
    /**
     * @brief Zdarzenie rozglaszane po odebraniu pakietu HEADER_GC_QUEST_INFO.
     * Informuje domene o nowym "scrollu" misji lub aktualizacji istniejacego Questa.
     */
    struct QuestScrollEvent : public UserInterface::Core::IEvent
    {
        uint16_t index;
        bool isBegin;
        bool isUpdate;
        bool isEnd;
        std::optional<std::string> title;
        std::optional<std::string> clockName;
        std::optional<int32_t> clockValue;
        std::optional<std::string> counterName;
        std::optional<int32_t> counterValue;
        std::optional<std::string> iconFileName;
    };

    /**
     * @brief Odczytuje pakiet Quest Info i powiadamia domene o zmianach (np. nowy scroll misji).
     * 
     * @param buffer Czysty bufor danych pakietu od serwera.
     * @return EterBase::PacketResult<void> Zwraca blad jesli pakiet jest uszkodzony.
     */
    EterBase::PacketResult<void> DispatchQuestInfo(std::span<const uint8_t> buffer)
    {
        size_t offset = 0;

        auto SafeRead = [&]<typename T>(T& out) -> bool {
            if (offset + sizeof(T) > buffer.size())
                return false;
            std::memcpy(&out, buffer.data() + offset, sizeof(T));
            offset += sizeof(T);
            return true;
        };

        auto SafeReadString = [&](size_t maxLength, std::string& outStr) -> bool {
            if (offset + maxLength > buffer.size())
                return false;
            
            // Scisle czytanie bez zakladania null-terminatora na koncu
            std::string_view view(reinterpret_cast<const char*>(buffer.data() + offset), maxLength);
            size_t nullPos = view.find('\0');
            if (nullPos != std::string_view::npos) {
                outStr = std::string(view.substr(0, nullPos));
            } else {
                outStr = std::string(view);
            }

            offset += maxLength;
            return true;
        };

        TPacketGCQuestInfo questInfo;
        if (!SafeRead(questInfo))
        {
            EterBase::ModernLogger::Error("DispatchQuestInfo: Buffer underflow while reading TPacketGCQuestInfo header.");
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        QuestScrollEvent event;
        event.index = questInfo.index;
        event.isBegin = false;
        event.isUpdate = false;
        event.isEnd = false;

        const uint8_t flag = questInfo.flag;

        if ((flag & QUEST_SEND_IS_BEGIN) != 0)
        {
            uint8_t isBeginVal = 0;
            if (!SafeRead(isBeginVal))
            {
                EterBase::ModernLogger::Error("DispatchQuestInfo: Buffer underflow while reading isBeginVal.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }
            if (isBeginVal)
            {
                event.isBegin = true;
            }
            else
            {
                event.isEnd = true;
            }
        }
        else
        {
            event.isUpdate = true;
        }

        if ((flag & QUEST_SEND_TITLE) != 0)
        {
            std::string title;
            if (!SafeReadString(30, title))
            {
                EterBase::ModernLogger::Error("DispatchQuestInfo: Buffer underflow while reading title.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }
            event.title = title;
        }
        
        if ((flag & QUEST_SEND_CLOCK_NAME) != 0)
        {
            std::string clockName;
            if (!SafeReadString(16, clockName))
            {
                EterBase::ModernLogger::Error("DispatchQuestInfo: Buffer underflow while reading clockName.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }
            event.clockName = clockName;
        }

        if ((flag & QUEST_SEND_CLOCK_VALUE) != 0)
        {
            int32_t clockValue = 0;
            if (!SafeRead(clockValue))
            {
                EterBase::ModernLogger::Error("DispatchQuestInfo: Buffer underflow while reading clockValue.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }
            event.clockValue = clockValue;
        }

        if ((flag & QUEST_SEND_COUNTER_NAME) != 0)
        {
            std::string counterName;
            if (!SafeReadString(16, counterName))
            {
                EterBase::ModernLogger::Error("DispatchQuestInfo: Buffer underflow while reading counterName.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }
            event.counterName = counterName;
        }

        if ((flag & QUEST_SEND_COUNTER_VALUE) != 0)
        {
            int32_t counterValue = 0;
            if (!SafeRead(counterValue))
            {
                EterBase::ModernLogger::Error("DispatchQuestInfo: Buffer underflow while reading counterValue.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }
            event.counterValue = counterValue;
        }

        if ((flag & QUEST_SEND_ICON_FILE) != 0)
        {
            std::string iconFile;
            if (!SafeReadString(24, iconFile))
            {
                EterBase::ModernLogger::Error("DispatchQuestInfo: Buffer underflow while reading iconFileName.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }
            event.iconFileName = iconFile;
        }

        EterBase::ModernLogger::Info("DispatchQuestInfo: Quest {} parsed. Begin: {}, Update: {}, End: {}", 
            event.index, event.isBegin, event.isUpdate, event.isEnd);

        // Wysylamy event na EventBus - dekompozycja od UI i Pythona.
        UserInterface::Core::EventBus::GetInstance().Publish(event);

        return {};
    }
}
