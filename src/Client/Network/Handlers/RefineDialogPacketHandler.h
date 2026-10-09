#pragma once

#include <span>
#include <cstdint>
#include <vector>
#include "EterBase/Result.h"
#include "Client/Network/Protocol/Protocol.h"

namespace Client::Network::Handlers {

// Struktura opisujaca wymagany material do ulepszenia przedmiotu
struct RefineMaterial {
    uint32_t vnum;
    int32_t count;
};

// Struktura zawierajaca informacje o oknie ulepszania
struct RefineDialogInfo {
    uint8_t type;
    uint8_t pos;
    uint32_t src_vnum;
    uint32_t result_vnum;
    int32_t cost;
    int32_t prob;
    std::vector<RefineMaterial> materials;
};

// Klasa odpowiedzialna za przetwarzanie pakietow okna ulepszania
class RefineDialogPacketHandler {
public:
    // Przetwarza stary pakiet z informacjami o ulepszaniu
    [[nodiscard]] static EterBase::PacketResult<RefineDialogInfo> HandleRefineInformationPacket(std::span<const uint8_t> payload);
    
    // Przetwarza nowy pakiet z informacjami o ulepszaniu
    [[nodiscard]] static EterBase::PacketResult<RefineDialogInfo> HandleRefineInformationNewPacket(std::span<const uint8_t> payload);
};

} // namespace Client::Network::Handlers
