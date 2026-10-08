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
        packet.header = CG::SAFEBOX_MONEY;
        packet.length = sizeof(TPacketCGSafeboxMoney);
        packet.bState = state;
        packet.lMoney = money;

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));
        return buffer;
    }

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> SafeboxCommandEncoder::EncodeCheckin(EterBase::ItemSlot inventorySlot, uint8_t safeboxSlot)
    {
        TPacketCGSafeboxCheckin packet{};
        packet.header = CG::SAFEBOX_CHECKIN;
        packet.length = sizeof(TPacketCGSafeboxCheckin);
        packet.bSafePos = safeboxSlot;
        packet.ItemPos = TItemPos(INVENTORY, inventorySlot.Value());

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));
        return buffer;
    }

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> SafeboxCommandEncoder::EncodeCheckout(uint8_t safeboxSlot, EterBase::ItemSlot inventorySlot)
    {
        TPacketCGSafeboxCheckout packet{};
        packet.header = CG::SAFEBOX_CHECKOUT;
        packet.length = sizeof(TPacketCGSafeboxCheckout);
        packet.bSafePos = safeboxSlot;
        packet.ItemPos = TItemPos(INVENTORY, inventorySlot.Value());

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));
        return buffer;
    }

    [[nodiscard]] EterBase::PacketResult<std::vector<uint8_t>> SafeboxCommandEncoder::EncodeItemMove(uint8_t sourceSlot, uint8_t targetSlot, uint8_t count)
    {
        TPacketCGItemMove packet{};
        packet.header = CG::SAFEBOX_ITEM_MOVE;
        packet.length = sizeof(TPacketCGItemMove);
        packet.pos = TItemPos(SAFEBOX, sourceSlot);
        packet.change_pos = TItemPos(SAFEBOX, targetSlot);
        packet.num = count;

        std::vector<uint8_t> buffer(sizeof(packet));
        std::memcpy(buffer.data(), &packet, sizeof(packet));
        return buffer;
    }
}

