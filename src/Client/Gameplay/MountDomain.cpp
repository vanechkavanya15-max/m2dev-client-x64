#include "../../StdAfx.h"
#include "MountDomain.h"
#include "../../EterBase/ModernLogger.h"
#include "../../GameLib/RaceMotionData.h"

namespace Client::Gameplay
{
    namespace
    {
        constexpr uint32_t BASE_SPEED_BONUS = 30; // 30 is a base proxy
        
        bool IsHorseMotionMode(uint32_t mode)
        {
            switch (mode)
            {
                case CRaceMotionData::MODE_HORSE:
                case CRaceMotionData::MODE_HORSE_ONEHAND_SWORD:
                case CRaceMotionData::MODE_HORSE_TWOHAND_SWORD:
                case CRaceMotionData::MODE_HORSE_DUALHAND_SWORD:
                case CRaceMotionData::MODE_HORSE_BOW:
                case CRaceMotionData::MODE_HORSE_FAN:
                case CRaceMotionData::MODE_HORSE_BELL:
                    return true;
                default:
                    return false;
            }
        }
    }

    class MountDomainImpl : public IMountDomain
    {
    public:
        MountDomainImpl() : m_isMounting(false), m_mountVnum(0) {}

        std::expected<void, std::string> Mount(uint32_t mountVnum) override
        {
            if (m_isMounting)
            {
                EterBase::ModernLogger::Error("MountDomain::Mount - Already mounting.");
                return std::unexpected("Already mounting");
            }
            if (mountVnum == 0)
            {
                EterBase::ModernLogger::Error("MountDomain::Mount - Invalid mount VNUM.");
                return std::unexpected("Invalid mount VNUM");
            }

            m_isMounting = true;
            m_mountVnum = mountVnum;

            EterBase::ModernLogger::Info("MountDomain::Mount - Started mounting on {}", mountVnum);
            return {};
        }

        std::expected<void, std::string> Dismount() override
        {
            if (!m_isMounting)
            {
                EterBase::ModernLogger::Error("MountDomain::Dismount - Not currently mounting.");
                return std::unexpected("Not currently mounting");
            }

            m_isMounting = false;
            m_mountVnum = 0;

            EterBase::ModernLogger::Info("MountDomain::Dismount - Stopped mounting.");
            return {};
        }

        bool IsMounting() const override
        {
            return m_isMounting;
        }

        uint32_t GetMountLevel() const override
        {
            if (!m_isMounting)
                return 0;

            switch (m_mountVnum)
            {
                case 20101:
                case 20102:
                case 20103:
                    return 1;
                case 20104:
                case 20105:
                case 20106:
                    return 2;
                case 20107:
                case 20108:
                case 20109:
                case 20110:
                case 20111:
                case 20112:
                case 20113:
                case 20114:
                case 20115:
                case 20116:
                case 20117:
                case 20118:
                case 20120:
                case 20121:
                case 20122:
                case 20123:
                case 20124:
                case 20125:
                    return 3;
                case 20119:
                case 20219:
                case 20220:
                    return 2;
                default:
                    // Nowe wierzchowce lub nieznane przyjmujemy domyslnie jako lv 3 (lub wiecej)
                    return 3;
            }
        }

        uint32_t GetSpeedBonus() const override
        {
            if (!m_isMounting)
                return 0;

            uint32_t level = GetMountLevel();
            return level * BASE_SPEED_BONUS;
        }

        std::expected<void, std::string> CanChangeMotionMode(uint32_t newMode) const override
        {
            bool isNewModeHorse = IsHorseMotionMode(newMode);

            if (!m_isMounting && isNewModeHorse)
            {
                EterBase::ModernLogger::Error("MountDomain::CanChangeMotionMode - Cannot change to horse mode without mounting.");
                return std::unexpected("Cannot change to horse mode without mounting");
            }

            if (m_isMounting && !isNewModeHorse)
            {
                EterBase::ModernLogger::Error("MountDomain::CanChangeMotionMode - Cannot change to non-horse mode while mounting.");
                return std::unexpected("Cannot change to non-horse mode while mounting");
            }

            return {};
        }

    private:
        bool m_isMounting;
        uint32_t m_mountVnum;
    };

    std::unique_ptr<IMountDomain> CreateMountDomain()
    {
        return std::make_unique<MountDomainImpl>();
    }

} // namespace Client::Gameplay
