#pragma once

#include <string>
#include <vector>
#include <format>
#include <span>
#include <optional>
#include "EterBase/StrongTypes.h"
#include "EterBase/Result.h"

namespace UserInterface::GroundDrop
{
    /**
     * @brief Formats sockets (spirit stones) and their values in item tooltips.
     * 
     * Applies Single Responsibility Principle in C++23. Constructs textual 
     * tooltip details about socket slots and their currently socketed items.
     */
    class SocketFormatter
    {
    public:
        /**
         * @brief Formats the item's sockets block.
         * 
         * @param itemVnum The identifier of the base item that has sockets.
         * @param sockets  Span containing values of the sockets (stone vnums or limits).
         * @return EterBase::Result<std::string> Formatted text for the socket section.
         */
        static EterBase::Result<std::string> FormatSockets(
            EterBase::ItemVnum itemVnum, 
            std::span<const int32_t> sockets);
    };
}
