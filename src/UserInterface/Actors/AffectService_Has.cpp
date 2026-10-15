#include "../StdAfx.h"
#include "ICharacterAffectService.h"
#include "../Packet.h"
#include "EterBase/LogModern.h"
#include "EterBase/Result.h"

#include <unordered_map>
#include <array>

namespace UserInterface::Actors
{
    // Global state shared across the AffectService multi-file architecture
    std::unordered_map<EterBase::EntityId, std::array<uint32_t, 2>> g_actorAffects;

    /**
     * @brief Blyskawiczne sprawdzenie czy aktor posiada dany afekt (np. Trucizna).
     * @param id Identyfikator aktora.
     * @param affectIndex Indeks afektu (0-63).
     * @return true jesli posiada, false w przeciwnym razie lub blad z domeny.
     */
    std::expected<bool, EterBase::EntityError> AffectHasCheck(
        EterBase::EntityId id,
        uint32_t affectIndex)
    {
        if (!id)
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "AffectHasCheck failed: invalid EntityId {}", id.value());
            return std::unexpected(EterBase::EntityError::NotFound);
        }

        if (affectIndex >= 64)
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Warning, "AffectHasCheck warning: affectIndex {} out of range for entity {}", affectIndex, id.value());
            return std::unexpected(EterBase::EntityError::OutOfRange);
        }

        auto it = g_actorAffects.find(id);
        if (it == g_actorAffects.end())
        {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "AffectHasCheck for entity {}: false (not found in map)", id.value());
            return false;
        }

        uint32_t arrayIndex = affectIndex / 32;
        uint32_t bitIndex = affectIndex % 32;

        bool hasAffect = (it->second[arrayIndex] & (1u << bitIndex)) != 0;
        
        EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "AffectHasCheck: Entity {} has affect {}? {}", id.value(), affectIndex, hasAffect);
        
        return hasAffect;
    }
} // namespace UserInterface::Actors
