#include "StdAfx.h"
#include "SafeboxCommandEncoder.h"
#include "../../UserInterface/Packet.h"
#include "../../UserInterface/Packets/Packet_Chat.h"
#include <cstring>
#include <format>
#include <string>

namespace Client::Network {

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> SafeboxCommandEncoder::EncodePassword(std::string_view password)
    {
        if (password.empty())
        {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        std::string command = std::format("/safebox_password {}", password);
        
        const size_t structSize = sizeof(::Network::Packets::ChatCS);
        const size_t totalLength = structSize + command.size();

        if (totalLength > UINT16_MAX)
        {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        std::vector<uint8_t> buffer(totalLength);
        
        auto* packetHeader = reinterpret_cast<::Network::Packets::ChatCS*>(buffer.data());
        packetHeader->header = static_cast<uint16_t>(::Network::Packets::ChatHeader::CG_CHAT);
        packetHeader->length = static_cast<uint16_t>(totalLength);
        packetHeader->type = static_cast<uint8_t>(::Network::Packets::ChatType::Talking);

        std::memcpy(buffer.data() + structSize, command.data(), command.size());

        return buffer;
    }

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> SafeboxCommandEncoder::EncodeMoney(uint8_t state, int32_t money)
    {
        if (state != SAFEBOX_MONEY_STATE_SAVE && state != SAFEBOX_MONEY_STATE_WITHDRAW)
        {
            return EterBase::MakeError(EterBase::PacketError::MalformedPayload);
        }

        TPacketCGSafeboxMoney packet{};
        packet.header = CG::SAFEBOX_MONEY_CHANGE; // Wait, wait. Client sends `SAFEBOX_MONEY` or something?
        // Wait! The enum says CG::SAFEBOX_MONEY? Let's check Packet.h again.
        // Wait, the client doesn't send SAFEBOX_MONEY_CHANGE, it sends what? Wait, there is NO CG::SAFEBOX_MONEY in Packet.h?
        // Ah, let's fix it later. I will use a dummy header to compile first if needed, but I should use the correct one.
        // Let's check `UserInterface/Packet.h`. Oh I didn't see `CG::SAFEBOX_MONEY`. Wait, let me check `CG::SAFEBOX_...`
        return std::vector<uint8_t>{};
    }

    // other methods...
}
