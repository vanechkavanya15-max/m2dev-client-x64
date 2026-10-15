#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include "../src/Client/Network/Handlers/PlayerPointsHandler.h"

using namespace Network::Handlers;

// Dummy implementation of IPlayerStatsService for testing
namespace UserInterface::Services
{
    class MockPlayerStatsService : public IPlayerStatsService
    {
    public:
        PlayerPointsView view{};
        
        void SetPoint(uint32_t type, int64_t value) override
        {
            switch (type)
            {
                case POINT_HP: view.hp = static_cast<uint32_t>(value); break;
                case POINT_MAX_HP: view.maxHp = static_cast<uint32_t>(value); break;
                case POINT_SP: view.sp = static_cast<uint32_t>(value); break;
                case POINT_MAX_SP: view.maxSp = static_cast<uint32_t>(value); break;
                case POINT_EXP: view.exp = static_cast<uint64_t>(value); break;
                case POINT_GOLD: view.gold = value; break;
                default: break;
            }
        }
        
        int64_t GetPoint(uint32_t type) const override { return 0; }
        const PlayerPointsView& GetPoints() const override { return view; }
        void Clear() override {}
    };
}

void Test_PlayerPointsHandler_Valid()
{
    UserInterface::Services::MockPlayerStatsService mockService;
    
    TPacketGCPoints packet{};
    std::memset(packet.points, 0, sizeof(packet.points));
    packet.header = 0x11; // GC::POINTS
    packet.length = sizeof(TPacketGCPoints);
    packet.points[POINT_HP] = 100;
    packet.points[POINT_MAX_HP] = 500;
    packet.points[POINT_SP] = 50;
    packet.points[POINT_MAX_SP] = 200;
    packet.points[POINT_EXP] = 1000;
    packet.points[POINT_GOLD] = 5000;

    std::vector<uint8_t> buffer(sizeof(TPacketGCPoints));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCPoints));
    std::span<const uint8_t> payload(buffer);

    auto result = PlayerPointsHandler::HandlePoints(payload, mockService);
    assert(result.has_value());

    assert(mockService.view.hp == 100);
    assert(mockService.view.maxHp == 500);
    assert(mockService.view.sp == 50);
    assert(mockService.view.maxSp == 200);
    assert(mockService.view.exp == 1000);
    assert(mockService.view.gold == 5000);
}

void Test_PlayerPointsHandler_DivByZero_Protection()
{
    UserInterface::Services::MockPlayerStatsService mockService;
    
    TPacketGCPoints packet{};
    std::memset(packet.points, 0, sizeof(packet.points));
    packet.header = 0x11;
    packet.length = sizeof(TPacketGCPoints);
    packet.points[POINT_HP] = 10;
    packet.points[POINT_MAX_HP] = 0; // Should be fixed to 1
    packet.points[POINT_SP] = 0;
    packet.points[POINT_MAX_SP] = -10; // Should be fixed to 1
    packet.points[POINT_EXP] = 0;
    packet.points[POINT_GOLD] = 0;

    std::vector<uint8_t> buffer(sizeof(TPacketGCPoints));
    std::memcpy(buffer.data(), &packet, sizeof(TPacketGCPoints));
    std::span<const uint8_t> payload(buffer);

    auto result = PlayerPointsHandler::HandlePoints(payload, mockService);
    assert(result.has_value());

    assert(mockService.view.maxHp == 1);
    assert(mockService.view.maxSp == 1);
}

void Test_PlayerPointsHandler_BufferUnderflow()
{
    UserInterface::Services::MockPlayerStatsService mockService;
    
    std::vector<uint8_t> shortBuffer(sizeof(TPacketGCPoints) - 1, 0);
    std::span<const uint8_t> payload(shortBuffer);

    auto result = PlayerPointsHandler::HandlePoints(payload, mockService);
    assert(!result.has_value());
    assert(result.error() == EterBase::PacketError::BufferUnderflow);
}

int main()
{
    Test_PlayerPointsHandler_Valid();
    Test_PlayerPointsHandler_DivByZero_Protection();
    Test_PlayerPointsHandler_BufferUnderflow();

    std::cout << "All PlayerPointsHandler tests passed successfully!" << std::endl;
    return 0;
}
