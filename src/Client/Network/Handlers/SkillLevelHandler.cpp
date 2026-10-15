#include "SkillLevelHandler.h"

namespace Client::Network {

#pragma pack(push, 1)
    struct SkillLevelPacket {
        uint16_t header;
        uint16_t length;
        uint8_t abSkillLevels[255];
    };
#pragma pack(pop)

    SkillLevelHandler::SkillLevelHandler(Client::Gameplay::SkillDomain& skillDomain, IndexToSkillIdMapper mapper)
        : m_skillDomain(skillDomain), m_mapper(std::move(mapper))
    {
    }

    EterBase::PacketResult<void> SkillLevelHandler::HandleSkillLevel(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(SkillLevelPacket)) {
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const SkillLevelPacket*>(payload.data());

        for (uint16_t i = 0; i < 255; ++i) {
            uint8_t skillLevel = packet->abSkillLevels[i];
            
            auto mappedSkillId = m_mapper(static_cast<uint8_t>(i));
            if (mappedSkillId.has_value()) {
                m_skillDomain.SetSkillLevel(mappedSkillId.value(), skillLevel);
            }
        }

        return {};
    }

} // namespace Client::Network
