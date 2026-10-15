#include "StdAfx.h"
#include "CharAdditionalInfoHandler.h"
#include "../../NetworkActorManager.h"
#include "../../Packet.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace Network::Handlers
{
    EterBase::PacketResult<void> HandleCharAdditionalInfo(std::span<const uint8_t> payload, CNetworkActorManager& actorManager)
    {
        if (payload.size() != sizeof(TPacketGCCharacterAdditionalInfo))
        {
            EterBase::ModernLogger::Error("HandleCharAdditionalInfo: Invalid payload size. Expected {}, got {}", sizeof(TPacketGCCharacterAdditionalInfo), payload.size());
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        const auto* packet = reinterpret_cast<const TPacketGCCharacterAdditionalInfo*>(payload.data());

        // Extracting Strong Types for type safety.
        EterBase::EntityId entityId(packet->dwVID);
        EterBase::GuildId guildId(packet->dwGuildID);
        EterBase::ItemVnum mountVnum(packet->dwMountVnum);

        SNetworkUpdateActorData updateData;
        updateData.m_dwVID = entityId.value();
        updateData.m_dwGuildID = guildId.value();
        updateData.m_dwArmor = packet->awPart[CHR_EQUIPPART_ARMOR];
        updateData.m_dwWeapon = packet->awPart[CHR_EQUIPPART_WEAPON];
        updateData.m_dwHair = packet->awPart[CHR_EQUIPPART_HAIR];
        
        // Dodatkowe tytuly, szarfy i efekty gracza (titles, sashes, and effects)
        // Since TPacketGCCharacterAdditionalInfo doesn't have a title field, title is often handled via another packet
        // or embedded in name string format, e.g. "[Title] Name".
        // Sashes/acce and specific effects are also commonly encoded into specific `awPart` indices (like CHR_EQUIPPART_ACCE).
        // For C++23 zero-conflict architecture, we update all available fields provided by the struct.
        updateData.m_sAlignment = packet->sAlignment;
        updateData.m_byPKMode = packet->bPKMode;
        updateData.m_dwMountVnum = mountVnum.value();

        actorManager.UpdateActor(updateData);

        UserInterface::Core::EventBus::GetInstance().Publish(UserInterface::Core::TargetBoardRefreshEvent(entityId.value()));
        
        return {};
    }
}
