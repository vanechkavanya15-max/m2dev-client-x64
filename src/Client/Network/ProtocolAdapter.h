#pragma once

#include <EterBase/Result.h>
#include <cstdint>
#include <expected>
#include <string_view>

namespace Client::Network {

enum class ServerProfile : uint8_t {
    ClassicYmir = 0,
    Pandora = 1
};

class ProtocolAdapter {
public:
    explicit ProtocolAdapter(ServerProfile profile);

    // Przelicza opcode logiki gry (nasz, ujednolicony) na opcode serwera (wysylany na zewnatrz)
    [[nodiscard]] EterBase::PacketResult<uint16_t> TranslateClientToServer(uint16_t clientOpcode) const;

    // Przelicza opcode serwera (otrzymany ze swiata) na opcode logiki gry (uzywany wewnetrznie)
    [[nodiscard]] EterBase::PacketResult<uint16_t> TranslateServerToClient(uint16_t serverOpcode) const;

private:
    ServerProfile m_profile;
};

} // namespace Client::Network
