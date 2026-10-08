#include "Client/Gameplay/MovementTerrainCollider.h"
#include "EterBase/LogModern.h"
#include "Client/Core/StrongTypes.h"
#include "Client/Core/DomainCommands.h"
#include <cassert>
#include <vector>

using namespace Client::Gameplay;
using namespace Client::Core;
using namespace EterBase;

// Mock provider for testing
class MockTerrainAttributeProvider : public ITerrainAttributeProvider {
public:
    // Define a 1000x1000 map for tests, where each cell is 100 units
    uint8_t GetAttribute(const MapCoords& coords) const override {
        // Convert coords to simple grid logic (1 cell = 100 units)
        int cx = static_cast<int>(coords.x / 100.0f);
        int cy = static_cast<int>(coords.y / 100.0f);

        // Wall from x=500 to x=600, y=0 to y=1000 (cells x=5, y=0..9)
        if (cx == 5 && cy >= 0 && cy < 10) {
            return TERRAIN_ATTRIBUTE_BLOCK;
        }

        // Water pool at x=200..400, y=200..400 (cells x=2..3, y=2..3)
        if (cx >= 2 && cx <= 3 && cy >= 2 && cy <= 3) {
            return TERRAIN_ATTRIBUTE_WATER;
        }

        // L-shaped corner block at x=800..900, y=800..1000 and x=800..1000, y=800..900 (cells x=8,y=8..9 and x=8..9,y=8)
        if ((cx == 8 && cy >= 8 && cy <= 9) || (cx >= 8 && cx <= 9 && cy == 8)) {
            return TERRAIN_ATTRIBUTE_BLOCK;
        }

        return 0; // Free space
    }
};

void TestFreeMovement() {
    MockTerrainAttributeProvider provider;
    MovementTerrainCollider collider(&provider);

    MapCoords start{100.0f, 100.0f, 0.0f};
    MapCoords end{150.0f, 150.0f, 0.0f};

    auto result = collider.CalculateMovement(start, end);
    assert(result.has_value());
    assert(result.value().x == 150.0f);
    assert(result.value().y == 150.0f);
    
    ModernLogger::Info("TestFreeMovement passed.");
}

void TestWallSliding() {
    MockTerrainAttributeProvider provider;
    MovementTerrainCollider collider(&provider);

    // Starting at x=450, moving towards x=550 (through the wall at x=500)
    // Moving diagonally: y=100 to y=200
    MapCoords start{450.0f, 100.0f, 0.0f};
    MapCoords end{550.0f, 200.0f, 0.0f};

    auto result = collider.CalculateMovement(start, end);
    
    // We expect to slide along the Y axis. So X should stop before the wall, Y should reach target.
    // Last valid X would be somewhere around 450 + some steps before 500.
    // Our step size is 50, so it might step to x=500 (blocked), so last valid x is 450.
    // (Actual last valid depends on distance and step size interpolation)
    
    assert(result.has_value());
    // Since X is blocked, it should slide to Y = 200
    assert(result.value().y == 200.0f);
    // X should not reach 550
    assert(result.value().x < 500.0f);

    ModernLogger::Info("TestWallSliding passed.");
}

void TestWaterBlocking() {
    MockTerrainAttributeProvider provider;
    MovementTerrainCollider collider(&provider);

    // Starting at x=150, y=250, moving to x=350, y=250 (straight through water)
    MapCoords start{150.0f, 250.0f, 0.0f};
    MapCoords end{350.0f, 250.0f, 0.0f};

    auto result = collider.CalculateMovement(start, end);
    
    // Y is constant, so sliding Y won't help. X is blocked.
    // It should move as far as possible then stop.
    assert(result.has_value());
    assert(result.value().x < 250.0f); // Stopped before/at water
    assert(result.value().y == 250.0f);

    ModernLogger::Info("TestWaterBlocking passed.");
}

void TestCompletelyBlocked() {
    MockTerrainAttributeProvider provider;
    MovementTerrainCollider collider(&provider);

    // Start inside a block (e.g. wall at x=5)
    MapCoords start{550.0f, 100.0f, 0.0f};
    MapCoords end{650.0f, 100.0f, 0.0f};

    auto result = collider.CalculateMovement(start, end);
    
    assert(!result.has_value());
    assert(result.error() == CommandError::MovementBlocked);

    ModernLogger::Info("TestCompletelyBlocked passed.");
}

int main() {
    ModernLogger::Info("Starting MovementTerrainCollider tests...");
    
    TestFreeMovement();
    TestWallSliding();
    TestWaterBlocking();
    TestCompletelyBlocked();
    
    ModernLogger::Info("All MovementTerrainCollider tests passed successfully.");
    return 0;
}
