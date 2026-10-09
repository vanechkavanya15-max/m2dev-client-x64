#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "../../../EterBase/Result.h"
#include "../../Core/EventBus.h"

namespace Client::Network::Handlers
{
    /**
     * @brief Zdarzenie emitowane po odebraniu pakietu potwierdzenia questu (GC_QUEST_CONFIRM).
     */
    struct QuestConfirmEvent : public Client::Core::IEvent {
        std::string message;
        int32_t timeout;
        uint32_t requestPID;

        QuestConfirmEvent(std::string msg, int32_t t, uint32_t pid)
            : message(std::move(msg)), timeout(t), requestPID(pid) {}
    };

    /**
     * @brief Zdarzenie emitowane po odebraniu skryptu questa (GC_SCRIPT).
     */
    struct QuestScriptEvent : public Client::Core::IEvent {
        std::string scriptText;

        explicit QuestScriptEvent(std::string text)
            : scriptText(std::move(text)) {}
    };

    /**
     * @brief Handler zajmujacy sie parsowaniem pakietow dialogow i zadan (Zero-Conflict Standard).
     */
    class QuestDialogPacketHandler
    {
    public:
        /**
         * @brief Przetwarza pakiet potwierdzenia questa.
         * @param buffer Bufor z danymi pakietu.
         * @return Oczekiwany rezultat bezwartosciowy (std::expected) lub kod bledu parsowania.
         */
        [[nodiscard]] EterBase::PacketResult<void> HandleQuestConfirm(std::span<const uint8_t> buffer) const noexcept;

        /**
         * @brief Przetwarza pakiet skryptu questa.
         * @param buffer Bufor z danymi pakietu.
         * @return Oczekiwany rezultat bezwartosciowy (std::expected) lub kod bledu parsowania.
         */
        [[nodiscard]] EterBase::PacketResult<void> HandleQuestScript(std::span<const uint8_t> buffer) const noexcept;
    };
} // namespace Client::Network::Handlers
