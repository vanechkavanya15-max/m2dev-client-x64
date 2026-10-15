#pragma once

#include "Client/Network/Protocol/IProtocolProfile.h"
#include <cstdint>
#include <unordered_map>

namespace Network::Protocol::Drivers
{
    /**
     * @brief Profil protokolu serwerowego dla Beavium x64.
     * Implementuje interfejs Network::Routers::IProtocolProfile.
     * 
     * Odpowiada za:
     * 1. Remapowanie naglowkow Beavium na ujednolicone kody GC klienta.
     * 2. Wykrywanie pakietow customowych (Aura, Voice, Switchbot itp.) i bezpieczne ich
     *    odrzucanie ze strumienia TCP bez desynchronizacji bufora.
     */
    class BeaviumProtocolProfile : public Network::Routers::IProtocolProfile
    {
    public:
        BeaviumProtocolProfile();
        ~BeaviumProtocolProfile() override = default;

        [[nodiscard]] uint8_t RemapHeader(uint8_t rawHeader) const override;
        [[nodiscard]] bool IsDiscardHeader(uint8_t rawHeader) const override;
        [[nodiscard]] int GetDiscardSize(uint8_t rawHeader) const override;

    private:
        std::unordered_map<uint8_t, uint8_t> m_remapTable;
        std::unordered_map<uint8_t, int> m_discardTable;
    };
}
