#include "../../StdAfx.h"
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <array>
#include <cstring>
#include <expected>

#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "../../Core/EventBus.h"
#include "../../Packet.h"

namespace Network::Dispatchers
{
    /**
     * @brief Zdarzenie rozszerzonych informacji o innej postaci wysylane na EventBus.
     */
    struct CharacterAdditionalInfoEvent : public UserInterface::Core::IEvent
    {
        EterBase::EntityId vid;
        std::string name;
        std::array<uint16_t, CHR_EQUIPPART_NUM> parts;
        uint8_t empire;
        uint32_t guildId;
        uint32_t level;
        int16_t alignment;
        uint8_t pkMode;
        EterBase::ItemVnum mountVnum;

        CharacterAdditionalInfoEvent(
            EterBase::EntityId vid, 
            std::string_view nameView, 
            const uint16_t* pParts, 
            uint8_t empire, 
            uint32_t guildId, 
            uint32_t level, 
            int16_t alignment, 
            uint8_t pkMode, 
            EterBase::ItemVnum mountVnum)
            : vid(vid), 
              name(nameView), 
              empire(empire), 
              guildId(guildId), 
              level(level), 
              alignment(alignment), 
              pkMode(pkMode), 
              mountVnum(mountVnum) 
        {
            if (pParts) {
                std::memcpy(parts.data(), pParts, sizeof(uint16_t) * CHR_EQUIPPART_NUM);
            } else {
                parts.fill(0);
            }
        }
    };

    /**
     * @brief Przetwarza pakiet rozszerzonych informacji o postaci.
     * @param buffer Bufor bajtow pakietu (std::span).
     * @return EterBase::PacketResult<void> ze statusem powodzenia lub porazki.
     */
    EterBase::PacketResult<void> ProcessCharacterAdditionalInfo(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(TPacketGCCharacterAdditionalInfo)) {
            EterBase::ModernLogger::Error("ProcessCharacterAdditionalInfo: Buffer underflow. Expected {}, got {}", 
                sizeof(TPacketGCCharacterAdditionalInfo), buffer.size());
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCCharacterAdditionalInfo*>(buffer.data());

        if (packet->header != GC::CHAR_ADDITIONAL_INFO) {
            EterBase::ModernLogger::Error("ProcessCharacterAdditionalInfo: Invalid header. Expected {}, got {}", 
                GC::CHAR_ADDITIONAL_INFO, packet->header);
            return std::unexpected(EterBase::PacketError::InvalidHeader);
        }

        // Safely extract the potentially non-null-terminated string
        size_t nameLen = 0;
        while (nameLen < CHARACTER_NAME_MAX_LEN && packet->name[nameLen] != '\0') {
            nameLen++;
        }
        std::string_view nameView(packet->name, nameLen);

        EterBase::EntityId vid{packet->dwVID};
        EterBase::ItemVnum mountVnum{packet->dwMountVnum};

        UserInterface::Core::EventBus::GetInstance().Publish(
            CharacterAdditionalInfoEvent(
                vid,
                nameView,
                packet->awPart,
                packet->bEmpire,
                packet->dwGuildID,
                packet->dwLevel,
                packet->sAlignment,
                packet->bPKMode,
                mountVnum
            )
        );

        EterBase::ModernLogger::Debug("ProcessCharacterAdditionalInfo: Processed info for VID {}", packet->dwVID);

        return {};
    }
}
