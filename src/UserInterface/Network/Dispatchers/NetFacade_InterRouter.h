/**
 * @file NetFacade_InterRouter.h
 * @brief Modern C++23 Dispatcher for Dialogs, Shop, and Trade (Wave 4)
 * 
 * Zgodny z SRP, Zero-Conflict i nowoczesnym EterBase::Result.
 */

#pragma once

#include <span>
#include <cstdint>
#include "../../../EterBase/Result.h"

namespace Network::Dispatchers
{
    /**
     * @class NetFacade_InterRouter
     * @brief Odpowiada za rejestracje i wstepny routing pakietow dla handlu, sklepow oraz dialogow (quest).
     */
    class NetFacade_InterRouter
    {
    public:
        /**
         * @brief Rejestruje handlery nowoczesne C++23 w systemie PacketDispatcher.
         */
        static void RegisterHandlers();

    private:
        static EterBase::PacketResult<void> HandleShop(std::span<const uint8_t> payload);
        static EterBase::PacketResult<void> HandleExchange(std::span<const uint8_t> payload);
        static EterBase::PacketResult<void> HandleScript(std::span<const uint8_t> payload);
        static EterBase::PacketResult<void> HandleQuestConfirm(std::span<const uint8_t> payload);
        static EterBase::PacketResult<void> HandleQuestInfo(std::span<const uint8_t> payload);
    };
} // namespace Network::Dispatchers
