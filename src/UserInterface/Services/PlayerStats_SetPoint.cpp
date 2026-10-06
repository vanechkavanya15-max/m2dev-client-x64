#include "../StdAfx.h"
#include "PlayerStatsService.h"
#include "../Packet.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Services
{
    void PlayerStatsService::SetPoint(uint32_t type, int64_t value)
    {
        if (type >= m_points.size())
            return;

        m_points[type] = value;

        switch (type)
        {
            case POINT_HP: m_view.hp = static_cast<uint32_t>(value); break;
            case POINT_MAX_HP: m_view.maxHp = static_cast<uint32_t>(value); break;
            case POINT_SP: m_view.sp = static_cast<uint32_t>(value); break;
            case POINT_MAX_SP: m_view.maxSp = static_cast<uint32_t>(value); break;
            case POINT_STAMINA: m_view.stamina = static_cast<uint32_t>(value); break;
            case POINT_MAX_STAMINA: m_view.maxStamina = static_cast<uint32_t>(value); break;
            case POINT_EXP: m_view.exp = static_cast<uint64_t>(value); break;
            case POINT_NEXT_EXP: m_view.nextExp = static_cast<uint64_t>(value); break;
            case POINT_GOLD: m_view.gold = value; break;
            case POINT_LEVEL: m_view.level = static_cast<uint8_t>(value); break;
            case POINT_STAT: m_view.statPoints = static_cast<uint16_t>(value); break;
            case POINT_SKILL: m_view.skillPoints = static_cast<uint16_t>(value); break;
            default: break;
        }

        EterBase::ModernLogger::Debug("PlayerStatsService::SetPoint: type {} value {}", type, value);
    }
}
