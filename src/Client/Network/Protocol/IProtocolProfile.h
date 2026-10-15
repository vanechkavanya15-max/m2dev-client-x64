#pragma once

#include <cstdint>

namespace Network::Protocol
{
    /**
     * @brief Czysty interfejs profilu protokolu serwerowego (Wzorzec IProtocolProfile).
     * Umozliwia translacje opkodow i ignorowanie pakietow specyficznych dla danego serwera.
     */
    class IProtocolProfile
    {
    public:
        virtual ~IProtocolProfile() = default;
        virtual uint8_t RemapHeader(uint8_t rawHeader) const = 0;
        virtual bool IsDiscardHeader(uint8_t rawHeader) const = 0;
        virtual int GetDiscardSize(uint8_t rawHeader) const = 0;
    };

    /**
     * @brief Domyslna implementacja profilu protokolu (DefaultProtocolProfile).
     * RemapHeader zwraca rawHeader, a IsDiscardHeader zwraca false.
     */
    class DefaultProtocolProfile : public IProtocolProfile
    {
    public:
        ~DefaultProtocolProfile() override = default;
        uint8_t RemapHeader(uint8_t rawHeader) const override { return rawHeader; }
        bool IsDiscardHeader([[maybe_unused]] uint8_t rawHeader) const override { return false; }
        int GetDiscardSize([[maybe_unused]] uint8_t rawHeader) const override { return 0; }
    };
}

namespace UserInterface::Network::Routers
{
    using IProtocolProfile = ::Network::Protocol::IProtocolProfile;
    using DefaultProtocolProfile = ::Network::Protocol::DefaultProtocolProfile;
}

namespace Network::Routers
{
    using IProtocolProfile = ::Network::Protocol::IProtocolProfile;
    using DefaultProtocolProfile = ::Network::Protocol::DefaultProtocolProfile;
}
