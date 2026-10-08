#include "LoginPacketHandler.h"
#ifndef TEST_MODE_DISABLE_STDAFX
#include "StdAfx.h"
#endif

#ifdef TEST_MODE_DISABLE_STDAFX
namespace GC
{
    constexpr uint16_t LOGIN_SUCCESS4     = 0x0105;
    constexpr uint16_t LOGIN_FAILURE      = 0x0106;
    constexpr uint16_t AUTH_SUCCESS       = 0x0108;
}
#define CHARACTER_NAME_MAX_LEN 24
#define GUILD_NAME_MAX_LEN 12
#define PLAYER_PER_ACCOUNT4 4

typedef struct simple_player_information
{
    uint32_t               dwID;
    char                szName[CHARACTER_NAME_MAX_LEN + 1];
    uint8_t                byJob;
    uint8_t                byLevel;
    uint32_t               dwPlayMinutes;
    uint8_t                byST, byHT, byDX, byIQ;
    uint16_t                wMainPart;
    uint8_t                bChangeName;
    uint16_t                wHairPart;
    uint8_t                bDummy[4];
    int32_t             x, y;
    uint32_t                lAddr;
    uint16_t                wPort;
    uint8_t             bySkillGroup;
} TSimplePlayerInformation;

typedef struct packet_login_success4
{
    uint16_t    header;
    uint16_t    length;
    TSimplePlayerInformation    akSimplePlayerInformation[PLAYER_PER_ACCOUNT4];
    uint32_t                        guild_id[PLAYER_PER_ACCOUNT4];
    char                        guild_name[PLAYER_PER_ACCOUNT4][GUILD_NAME_MAX_LEN+1];
    uint32_t handle;
    uint32_t random_key;
} TPacketGCLoginSuccess4;

enum { LOGIN_STATUS_MAX_LEN = 8 };
typedef struct packet_login_failure
{
    uint16_t    header;
    uint16_t    length;
    char    szStatus[LOGIN_STATUS_MAX_LEN + 1];
} TPacketGCLoginFailure;

typedef struct packet_auth_success
{
    uint16_t    header;
    uint16_t    length;
    uint32_t       dwLoginKey;
    uint8_t        bResult;
} TPacketGCAuthSuccess;
#endif

#include <cstring>
#include <algorithm>

namespace Client::Network
{
    LoginPacketHandler::LoginPacketHandler(ILoginCallback* callback)
        : m_callback(callback)
    {
    }

    EterBase::PacketResult<void> LoginPacketHandler::Handle(std::span<const uint8_t> payload)
    {
        if (payload.empty())
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        uint16_t header = 0;
        if (payload.size() >= sizeof(uint16_t))
        {
            header = *reinterpret_cast<const uint16_t*>(payload.data());
        }
        else
        {
            header = payload[0];
        }

        switch (header)
        {
            case GC::LOGIN_SUCCESS4:
                return HandleLoginSuccess4(payload);
            case GC::LOGIN_FAILURE:
                return HandleLoginFailure(payload);
            case GC::AUTH_SUCCESS:
                return HandleAuthSuccess(payload);
            default:
                return EterBase::MakeError(EterBase::PacketError::UnknownOpcode);
        }
    }

    uint16_t LoginPacketHandler::GetExpectedSize() const
    {
        return 0;
    }

    bool LoginPacketHandler::IsDynamicSize() const
    {
        return true;
    }

    EterBase::PacketResult<void> LoginPacketHandler::HandleLoginSuccess4(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCLoginSuccess4))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCLoginSuccess4*>(payload.data());

        LoginResult result;
        result.session.handle = packet->handle;
        result.session.randomKey = packet->random_key;
        result.session.loginKey = 0; // Not provided in this packet

        for (int i = 0; i < PLAYER_PER_ACCOUNT4; ++i)
        {
            PlayerInfo info;
            const auto& src = packet->akSimplePlayerInformation[i];
            
            info.id = src.dwID;
            info.name = std::string(src.szName, strnlen(src.szName, sizeof(src.szName)));
            info.job = src.byJob;
            info.level = src.byLevel;
            info.playMinutes = src.dwPlayMinutes;
            info.st = src.byST;
            info.ht = src.byHT;
            info.dx = src.byDX;
            info.iq = src.byIQ;
            info.mainPart = src.wMainPart;
            info.changeName = src.bChangeName;
            info.hairPart = src.wHairPart;
            info.x = src.x;
            info.y = src.y;
            info.addr = src.lAddr;
            info.port = src.wPort;
            info.skillGroup = src.bySkillGroup;

            info.guildId = packet->guild_id[i];
            info.guildName = std::string(packet->guild_name[i], strnlen(packet->guild_name[i], sizeof(packet->guild_name[i])));

            result.players.push_back(info);
        }

        if (m_callback)
        {
            m_callback->OnLoginSuccess(result);
        }

        return {};
    }

    EterBase::PacketResult<void> LoginPacketHandler::HandleLoginFailure(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCLoginFailure))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCLoginFailure*>(payload.data());

        AccountStatus status;
        status.status = std::string(packet->szStatus, strnlen(packet->szStatus, sizeof(packet->szStatus)));

        if (m_callback)
        {
            m_callback->OnLoginFailure(status);
        }

        return {};
    }

    EterBase::PacketResult<void> LoginPacketHandler::HandleAuthSuccess(std::span<const uint8_t> payload)
    {
        if (payload.size() < sizeof(TPacketGCAuthSuccess))
        {
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const TPacketGCAuthSuccess*>(payload.data());

        if (packet->bResult)
        {
            SessionKey key;
            key.loginKey = packet->dwLoginKey;
            key.handle = 0;
            key.randomKey = 0;

            if (m_callback)
            {
                m_callback->OnAuthSuccess(key);
            }
        }
        else
        {
            AccountStatus status;
            status.status = "BESAMEKEY";
            
            if (m_callback)
            {
                m_callback->OnLoginFailure(status);
            }
        }

        return {};
    }
}
