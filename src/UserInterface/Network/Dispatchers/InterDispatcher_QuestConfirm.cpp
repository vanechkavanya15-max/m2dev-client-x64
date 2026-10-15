#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"

#include <cstdint>
#include <span>
#include <string>
#include <cstring>
#include <algorithm>

namespace Network::Dispatchers
{
    /**
     * @brief Zdarzenie rozglaszane przez EventBus po odebraniu potwierdzenia misji.
     * Umozliwia odpiecie warstwy GUI od handlera sieciowego.
     */
    struct QuestConfirmReceivedEvent : public UserInterface::Core::IEvent
    {
        std::string message;
        int32_t timeout;
        uint32_t requestPid;

        QuestConfirmReceivedEvent(std::string msg, int32_t t, uint32_t pid)
            : message(std::move(msg)), timeout(t), requestPid(pid)
        {
        }
    };

    /**
     * @brief Przetwarza pakiet HEADER_GC_QUEST_CONFIRM i powiadamia GUI za posrednictwem EventBus.
     * 
     * Implementuje zasade Zero-Conflict - operuje tylko na buforze, nie wola metod Pythona.
     * 
     * @param buffer Bezpieczny widok na bufor pamieci C++23.
     * @return EterBase::PacketResult<void> Pusty sukces lub blad w przypadku uciecia bufora.
     */
    EterBase::PacketResult<void> DispatchQuestConfirm(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCQuestConfirm))
        {
            EterBase::ModernLogger::Error("DispatchQuestConfirm: Buffer underflow. Expected {}, got {}", sizeof(TPacketGCQuestConfirm), buffer.size());
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        TPacketGCQuestConfirm packet;
        std::memcpy(&packet, buffer.data(), sizeof(TPacketGCQuestConfirm));

        // Bezpieczne skopiowanie z gwarancja braku przeplnienia
        char safeMsg[sizeof(packet.msg) + 1] = {0};
        std::memcpy(safeMsg, packet.msg, sizeof(packet.msg));
        safeMsg[sizeof(packet.msg)] = '\0';
        std::string messageStr(safeMsg);

        EterBase::ModernLogger::Debug("DispatchQuestConfirm: Received confirm request. PID: {}, Timeout: {}", packet.requestPID, packet.timeout);

        UserInterface::Core::EventBus::GetInstance().Publish(
            QuestConfirmReceivedEvent{std::move(messageStr), packet.timeout, packet.requestPID}
        );

        return {};
    }
}
