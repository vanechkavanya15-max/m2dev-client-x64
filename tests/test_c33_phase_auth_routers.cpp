#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>
#include <cstring>

#include "../src/UserInterface/Contracts/IPacketRouter.h"
#include "../src/UserInterface/Contracts/IGameEvents.h"
#include "../src/UserInterface/Network/Routers/NetLoginRouter.h"
#include "../src/UserInterface/Network/Routers/NetSelectRouter.h"
#include "../src/UserInterface/Core/EventBus.h"

using namespace UserInterface::Contracts;
using namespace UserInterface::Network::Routers;

// Klasa Mock dla IGameEventSink do weryfikacji faz logowania i wyboru postaci
class MockAuthEventSink : public IGameEventSink
{
public:
    int loginSuccessCount = 0;
    LoginSuccessEvent lastLoginSuccess{};

    int loginFailureCount = 0;
    LoginFailureEvent lastLoginFailure{};

    int empireCount = 0;
    EmpireSelectedEvent lastEmpire{};

    int charCreateSuccessCount = 0;
    CharacterCreateSuccessEvent lastCreateSuccess{};

    int charCreateFailureCount = 0;
    CharacterCreateFailureEvent lastCreateFailure{};

    int charDeleteSuccessCount = 0;
    CharacterDeleteSuccessEvent lastDeleteSuccess{};

    void OnLoginSuccess(const LoginSuccessEvent& event) override
    {
        loginSuccessCount++;
        lastLoginSuccess = event;
    }

    void OnLoginFailure(const LoginFailureEvent& event) override
    {
        loginFailureCount++;
        lastLoginFailure = event;
    }

    void OnEmpireSelected(const EmpireSelectedEvent& event) override
    {
        empireCount++;
        lastEmpire = event;
    }

    void OnCharacterCreateSuccess(const CharacterCreateSuccessEvent& event) override
    {
        charCreateSuccessCount++;
        lastCreateSuccess = event;
    }

    void OnCharacterCreateFailure(const CharacterCreateFailureEvent& event) override
    {
        charCreateFailureCount++;
        lastCreateFailure = event;
    }

    void OnCharacterDeleteSuccess(const CharacterDeleteSuccessEvent& event) override
    {
        charDeleteSuccessCount++;
        lastDeleteSuccess = event;
    }
};

static void TestNetLoginRouter()
{
    std::cout << "[TEST] Rozpoczynanie testu NetLoginRouter...\n";

    MockAuthEventSink mockSink;
    NetLoginRouter router(&mockSink);

    assert(router.GetRouterName() == "NetLoginRouter");
    assert(router.GetEventSink() == &mockSink);

    // Sprawdzenie obslugi naglowkow legacy i nowoczesnych
    assert(router.CanHandleHeader(static_cast<uint8_t>(0x07)) == true); // LOGIN_SUCCESS3
    assert(router.CanHandleHeader(static_cast<uint8_t>(0x08)) == true); // LOGIN_FAILURE
    assert(router.CanHandleHeader(static_cast<uint8_t>(0x09)) == true); // LOGIN_KEY
    assert(router.CanHandleHeader(static_cast<uint8_t>(0x5A)) == true); // EMPIRE
    assert(router.CanHandleHeader(static_cast<uint8_t>(0xFF)) == false);

    assert(router.CanHandleHeader(Client::Network::Protocol::GC::LOGIN_SUCCESS3) == true);
    assert(router.CanHandleHeader(Client::Network::Protocol::GC::LOGIN_SUCCESS4) == true);
    assert(router.CanHandleHeader(Client::Network::Protocol::GC::LOGIN_FAILURE) == true);
    assert(router.CanHandleHeader(Client::Network::Protocol::GC::LOGIN_KEY) == true);
    assert(router.CanHandleHeader(Client::Network::Protocol::GC::AUTH_SUCCESS) == true);
    assert(router.CanHandleHeader(Client::Network::Protocol::GC::EMPIRE) == true);

    // Test HandleLoginSuccess3
    TPacketGCLoginSuccess3 pktSuccess3{};
    pktSuccess3.handle = 12345;
    pktSuccess3.random_key = 67890;
    router.HandleLoginSuccess3(pktSuccess3);

    assert(mockSink.loginSuccessCount == 1);
    assert(mockSink.lastLoginSuccess.dwHandle == 12345);
    assert(mockSink.lastLoginSuccess.dwRandomKey == 67890);
    assert(mockSink.lastLoginSuccess.dwPlayerCount == PLAYER_PER_ACCOUNT3);

    // Test HandleLoginSuccess4
    TPacketGCLoginSuccess4 pktSuccess4{};
    pktSuccess4.handle = 54321;
    pktSuccess4.random_key = 98765;
    router.HandleLoginSuccess4(pktSuccess4);

    assert(mockSink.loginSuccessCount == 2);
    assert(mockSink.lastLoginSuccess.dwHandle == 54321);
    assert(mockSink.lastLoginSuccess.dwRandomKey == 98765);
    assert(mockSink.lastLoginSuccess.dwPlayerCount == PLAYER_PER_ACCOUNT4);

    // Test HandleLoginFailure
    TPacketGCLoginFailure pktFailure{};
    std::strncpy(pktFailure.szStatus, "ALREADY", sizeof(pktFailure.szStatus) - 1);
    router.HandleLoginFailure(pktFailure);

    assert(mockSink.loginFailureCount == 1);
    assert(mockSink.lastLoginFailure.szStatus == "ALREADY");

    // Test HandleEmpire
    TPacketGCEmpire pktEmpire{};
    pktEmpire.bEmpire = 3;
    router.HandleEmpire(pktEmpire);

    assert(mockSink.empireCount == 1);
    assert(mockSink.lastEmpire.byEmpire == 3);

    std::cout << "[PASS] NetLoginRouter przetestowany pomyslnie.\n";
}

static void TestNetSelectRouter()
{
    std::cout << "[TEST] Rozpoczynanie testu NetSelectRouter...\n";

    MockAuthEventSink mockSink;
    NetSelectRouter router(&mockSink);

    assert(router.GetRouterName() == "NetSelectRouter");
    assert(router.GetEventSink() == &mockSink);

    // Sprawdzenie naglowkow
    assert(router.CanHandleHeader(static_cast<uint8_t>(0x11)) == true); // CREATE_SUCCESS
    assert(router.CanHandleHeader(static_cast<uint8_t>(0x12)) == true); // CREATE_FAILURE
    assert(router.CanHandleHeader(static_cast<uint8_t>(0x13)) == true); // DELETE_SUCCESS
    assert(router.CanHandleHeader(static_cast<uint8_t>(0x17)) == true); // CHANGE_NAME
    assert(router.CanHandleHeader(static_cast<uint8_t>(0x5A)) == true); // EMPIRE
    assert(router.CanHandleHeader(static_cast<uint8_t>(0xFE)) == false);

    assert(router.CanHandleHeader(Client::Network::Protocol::GC::PLAYER_CREATE_SUCCESS) == true);
    assert(router.CanHandleHeader(Client::Network::Protocol::GC::PLAYER_CREATE_FAILURE) == true);
    assert(router.CanHandleHeader(Client::Network::Protocol::GC::PLAYER_DELETE_SUCCESS) == true);
    assert(router.CanHandleHeader(Client::Network::Protocol::GC::CHANGE_NAME) == true);

    // Test HandlePlayerCreateSuccess
    TPacketGCPlayerCreateSuccess pktCreateSuccess{};
    pktCreateSuccess.bAccountCharacterSlot = 2;
    std::strncpy(pktCreateSuccess.kSimplePlayerInfomation.szName, "SuraMaster", CHARACTER_NAME_MAX_LEN);
    router.HandlePlayerCreateSuccess(pktCreateSuccess);

    assert(mockSink.charCreateSuccessCount == 1);
    assert(mockSink.lastCreateSuccess.bySlot == 2);
    assert(mockSink.lastCreateSuccess.szName == "SuraMaster");

    // Test HandlePlayerCreateFailure
    TPacketGCCreateFailure pktCreateFail{};
    pktCreateFail.bType = 5;
    router.HandlePlayerCreateFailure(pktCreateFail);

    assert(mockSink.charCreateFailureCount == 1);
    assert(mockSink.lastCreateFailure.byType == 5);

    // Test HandlePlayerDestroySuccess
    TPacketGCDestroyCharacterSuccess pktDelete{};
    pktDelete.account_index = 1;
    router.HandlePlayerDestroySuccess(pktDelete);

    assert(mockSink.charDeleteSuccessCount == 1);
    assert(mockSink.lastDeleteSuccess.bySlot == 1);

    // Test HandleEmpire
    TPacketGCEmpire pktEmpire{};
    pktEmpire.bEmpire = 1;
    router.HandleEmpire(pktEmpire);

    assert(mockSink.empireCount == 1);
    assert(mockSink.lastEmpire.byEmpire == 1);

    std::cout << "[PASS] NetSelectRouter przetestowany pomyslnie.\n";
}

static void TestEventBusIntegration()
{
    std::cout << "[TEST] Rozpoczynanie testu EventBus Integration dla fazy Auth...\n";

    bool bLoginBusHandled = false;
    uint32_t receivedHandle = 0;

    auto subId = UserInterface::Core::EventBus::Instance().Subscribe<NetLoginSuccessDomainEvent>(
        [&](const NetLoginSuccessDomainEvent& evt) {
            bLoginBusHandled = true;
            receivedHandle = evt.dwHandle;
        }
    );

    NetLoginRouter loginRouter(nullptr);
    TPacketGCLoginSuccess3 pkt{};
    pkt.handle = 999999;
    pkt.random_key = 111111;
    loginRouter.HandleLoginSuccess3(pkt);

    assert(bLoginBusHandled == true);
    assert(receivedHandle == 999999);

    UserInterface::Core::EventBus::Instance().Unsubscribe<NetLoginSuccessDomainEvent>(subId);
    std::cout << "[PASS] EventBus Integration przetestowany pomyslnie.\n";
}

int main()
{
    std::cout << "====================================================\n";
    std::cout << "  Start Testow Jednostkowych: PhaseAuthRoutersTest\n";
    std::cout << "====================================================\n";

    TestNetLoginRouter();
    TestNetSelectRouter();
    TestEventBusIntegration();

    std::cout << "====================================================\n";
    std::cout << "  [SUKCES] Wszystkie testy PhaseAuthRoutersTest PASS!\n";
    std::cout << "====================================================\n";
    return 0;
}
