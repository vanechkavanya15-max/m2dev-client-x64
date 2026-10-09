#pragma once

#include <cstdint>
#include <string_view>

namespace UserInterface::Contracts
{
    class IPacketRouter
    {
    public:
        virtual ~IPacketRouter() = default;

        virtual std::string_view GetRouterName() const noexcept = 0;
        virtual bool CanHandleHeader(uint8_t bHeader) const noexcept = 0;
    };
}
