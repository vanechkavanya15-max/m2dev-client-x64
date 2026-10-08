#include "StdAfx.h"
#include "MountCommandEncoder.h"
#include "Protocol/Protocol.h"
#include "EterBase/LogModern.h"
#include <vector>
#include <cstring>
#include <span>

namespace Client::Network {

    MountCommandEncoder::MountCommandEncoder(Client::Core::INetworkPort& port)
        : m_port(port)
    {
    }

    Client::Core::Result<void, Client::Core::PacketError> MountCommandEncoder::EncodeMount(TItemPos mountItem)
    {
        if (!mountItem.IsValidCell()) {
            EterBase::ModernLogger::Warn("MountCommandEncoder::EncodeMount - Invalid item cell.");
            return std::unexpected(Client::Core::PacketError::InvalidHeader); // Or another error mapping
        }

        TPacketCGItemUse packet{};
        packet.header = CG::ITEM_USE;
        packet.length = sizeof(packet);
        packet.pos = mountItem;

        auto payload = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        auto result = m_port.SendRaw(packet.header, payload);
        if (!result) {
            EterBase::ModernLogger::Error("MountCommandEncoder::EncodeMount - Failed to send packet.");
            return std::unexpected(result.error());
        }

        EterBase::ModernLogger::Debug("MountCommandEncoder::EncodeMount - Success.");
        return {};
    }

    Client::Core::Result<void, Client::Core::PacketError> MountCommandEncoder::EncodeDismount(TItemPos mountItem)
    {
        if (!mountItem.IsValidCell()) {
            EterBase::ModernLogger::Warn("MountCommandEncoder::EncodeDismount - Invalid item cell.");
            return std::unexpected(Client::Core::PacketError::InvalidHeader);
        }

        TPacketCGItemUse packet{};
        packet.header = CG::ITEM_USE;
        packet.length = sizeof(packet);
        packet.pos = mountItem;

        auto payload = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        auto result = m_port.SendRaw(packet.header, payload);
        if (!result) {
            EterBase::ModernLogger::Error("MountCommandEncoder::EncodeDismount - Failed to send packet.");
            return std::unexpected(result.error());
        }

        EterBase::ModernLogger::Debug("MountCommandEncoder::EncodeDismount - Success.");
        return {};
    }

    Client::Core::Result<void, Client::Core::PacketError> MountCommandEncoder::EncodeUseHorseSkill(EterBase::SkillId skillId, EterBase::EntityId targetVid)
    {
        TPacketCGUseSkill packet{};
        packet.header = CG::USE_SKILL;
        packet.length = sizeof(packet);
        packet.dwVnum = skillId.get();
        packet.dwTargetVID = targetVid.get();

        auto payload = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        auto result = m_port.SendRaw(packet.header, payload);
        if (!result) {
            EterBase::ModernLogger::Error("MountCommandEncoder::EncodeUseHorseSkill - Failed to send packet.");
            return std::unexpected(result.error());
        }

        EterBase::ModernLogger::Debug("MountCommandEncoder::EncodeUseHorseSkill - Success.");
        return {};
    }

    Client::Core::Result<void, Client::Core::PacketError> MountCommandEncoder::EncodeFeedMount(TItemPos foodPos, TItemPos mountPos)
    {
        if (!foodPos.IsValidCell() || !mountPos.IsValidCell()) {
            EterBase::ModernLogger::Warn("MountCommandEncoder::EncodeFeedMount - Invalid item cell.");
            return std::unexpected(Client::Core::PacketError::InvalidHeader);
        }

        TPacketCGItemUseToItem packet{};
        packet.header = CG::ITEM_USE_TO_ITEM;
        packet.length = sizeof(packet);
        packet.source_pos = foodPos;
        packet.target_pos = mountPos;

        auto payload = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
        auto result = m_port.SendRaw(packet.header, payload);
        if (!result) {
            EterBase::ModernLogger::Error("MountCommandEncoder::EncodeFeedMount - Failed to send packet.");
            return std::unexpected(result.error());
        }

        EterBase::ModernLogger::Debug("MountCommandEncoder::EncodeFeedMount - Success.");
        return {};
    }

} // namespace Client::Network
