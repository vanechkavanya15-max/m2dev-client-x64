#define TEST_MODE_DISABLE_STDAFX
#include <vector>
#include <cstring>
#include <iostream>
#include <cassert>
#include "../src/Client/Network/Handlers/LoginPacketHandler.h"

// Define missing types for testing if they are isolated
#ifndef _WIN32
#define DWORD uint32_t
#define BYTE uint8_t
#endif

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

using namespace Client::Network;

class MockLoginCallback : public ILoginCallback
{
public:
    LoginResult lastLoginResult;
    AccountStatus lastLoginFailure;
    SessionKey lastAuthSuccess;
    bool loginSuccessCalled = false;
    bool loginFailureCalled = false;
    bool authSuccessCalled = false;

    void OnLoginSuccess(const LoginResult& result) override
    {
        lastLoginResult = result;
        loginSuccessCalled = true;
    }

    void OnLoginFailure(const AccountStatus& status) override
    {
        lastLoginFailure = status;
        loginFailureCalled = true;
    }

    void OnAuthSuccess(const SessionKey& key) override
    {
        lastAuthSuccess = key;
        authSuccessCalled = true;
    }

    void Reset()
    {
        loginSuccessCalled = false;
        loginFailureCalled = false;
        authSuccessCalled = false;
    }
};

void TestHandleLoginSuccess4()
{
    MockLoginCallback callback;
    LoginPacketHandler handler(&callback);

    TPacketGCLoginSuccess4 packet = {};
    packet.header = GC::LOGIN_SUCCESS4;
    packet.length = sizeof(packet);
    packet.handle = 12345;
    packet.random_key = 67890;
    
    // Setup player 0
    packet.akSimplePlayerInformation[0].dwID = 1;
    strncpy(packet.akSimplePlayerInformation[0].szName, "TestPlayer", sizeof(packet.akSimplePlayerInformation[0].szName) - 1);
    packet.akSimplePlayerInformation[0].byJob = 1;
    packet.akSimplePlayerInformation[0].byLevel = 99;
    packet.guild_id[0] = 100;
    strncpy(packet.guild_name[0], "TestGuild", sizeof(packet.guild_name[0]) - 1);

    std::vector<uint8_t> payload(reinterpret_cast<uint8_t*>(&packet), reinterpret_cast<uint8_t*>(&packet) + sizeof(packet));

    auto result = handler.Handle(std::span<const uint8_t>(payload));

    assert(result.has_value());
    assert(callback.loginSuccessCalled);
    assert(callback.lastLoginResult.session.handle == 12345);
    assert(callback.lastLoginResult.session.randomKey == 67890);
    assert(callback.lastLoginResult.players.size() > 0);
    assert(callback.lastLoginResult.players[0].id == 1);
    assert(callback.lastLoginResult.players[0].name == "TestPlayer");
    assert(callback.lastLoginResult.players[0].job == 1);
    assert(callback.lastLoginResult.players[0].level == 99);
    assert(callback.lastLoginResult.players[0].guildId == 100);
    assert(callback.lastLoginResult.players[0].guildName == "TestGuild");
    std::cout << "TestHandleLoginSuccess4 passed.\n";
}

void TestHandleLoginFailure()
{
    MockLoginCallback callback;
    LoginPacketHandler handler(&callback);

    TPacketGCLoginFailure packet = {};
    packet.header = GC::LOGIN_FAILURE;
    packet.length = sizeof(packet);
    strncpy(packet.szStatus, "WRONGPWD", sizeof(packet.szStatus) - 1);

    std::vector<uint8_t> payload(reinterpret_cast<uint8_t*>(&packet), reinterpret_cast<uint8_t*>(&packet) + sizeof(packet));

    auto result = handler.Handle(std::span<const uint8_t>(payload));

    assert(result.has_value());
    assert(callback.loginFailureCalled);
    assert(callback.lastLoginFailure.status == "WRONGPWD");
    std::cout << "TestHandleLoginFailure passed.\n";
}

void TestHandleAuthSuccess_True()
{
    MockLoginCallback callback;
    LoginPacketHandler handler(&callback);

    TPacketGCAuthSuccess packet = {};
    packet.header = GC::AUTH_SUCCESS;
    packet.length = sizeof(packet);
    packet.dwLoginKey = 99999;
    packet.bResult = 1;

    std::vector<uint8_t> payload(reinterpret_cast<uint8_t*>(&packet), reinterpret_cast<uint8_t*>(&packet) + sizeof(packet));

    auto result = handler.Handle(std::span<const uint8_t>(payload));

    assert(result.has_value());
    assert(callback.authSuccessCalled);
    assert(callback.lastAuthSuccess.loginKey == 99999);
    std::cout << "TestHandleAuthSuccess_True passed.\n";
}

void TestHandleAuthSuccess_False()
{
    MockLoginCallback callback;
    LoginPacketHandler handler(&callback);

    TPacketGCAuthSuccess packet = {};
    packet.header = GC::AUTH_SUCCESS;
    packet.length = sizeof(packet);
    packet.dwLoginKey = 99999;
    packet.bResult = 0;

    std::vector<uint8_t> payload(reinterpret_cast<uint8_t*>(&packet), reinterpret_cast<uint8_t*>(&packet) + sizeof(packet));

    auto result = handler.Handle(std::span<const uint8_t>(payload));

    assert(result.has_value());
    assert(callback.loginFailureCalled);
    assert(callback.lastLoginFailure.status == "BESAMEKEY");
    std::cout << "TestHandleAuthSuccess_False passed.\n";
}

int main()
{
    TestHandleLoginSuccess4();
    TestHandleLoginFailure();
    TestHandleAuthSuccess_True();
    TestHandleAuthSuccess_False();
    std::cout << "All tests passed.\n";
    return 0;
}
