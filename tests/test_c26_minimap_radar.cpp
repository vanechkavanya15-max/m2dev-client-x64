#include <iostream>
#include <cassert>
#include <cmath>
#include "../src/Client/UI/MiniMapRadar.h"

// Simple test framework macro
#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "..." << std::endl; \
        test_func(); \
        std::cout << #test_func << " PASSED!" << std::endl; \
    } while (0)

using namespace Client::UI;

void TestColors() {
    assert(MiniMapRadar::GetMarkColor(RadarMarkType::NPC) == 0xFF7AE75D);
    assert(MiniMapRadar::GetMarkColor(RadarMarkType::Party) == 0xFF3388FF);
    assert(MiniMapRadar::GetMarkColor(RadarMarkType::Warp) == 0xFFCC33CC);
    assert(MiniMapRadar::GetMarkColor(RadarMarkType::Mob) == 0xFFFF3333);
    assert(MiniMapRadar::GetMarkColor(RadarMarkType::Waypoint) == 0xFFFFFF00);
}

void TestProjectionInside() {
    MiniMapRadar radar;
    radar.SetCenterPosition(0.0f, 0.0f);
    radar.SetRadius(100.0f);
    radar.SetScale(1.0f);
    
    float outX, outY;
    bool inside = radar.ProjectToRadar(50.0f, 50.0f, outX, outY);
    
    assert(inside == true);
    assert(std::abs(outX - 50.0f) < 0.001f);
    assert(std::abs(outY - 50.0f) < 0.001f);
}

void TestProjectionOutside() {
    MiniMapRadar radar;
    radar.SetCenterPosition(0.0f, 0.0f);
    radar.SetRadius(100.0f);
    radar.SetScale(1.0f);
    
    float outX, outY;
    // Punkt 200, 0 jest poza, powinien zostac przyciety do 100, 0
    bool inside = radar.ProjectToRadar(200.0f, 0.0f, outX, outY);
    
    assert(inside == false);
    assert(std::abs(outX - 100.0f) < 0.001f);
    assert(std::abs(outY - 0.0f) < 0.001f);
}

void TestProjectionWithScale() {
    MiniMapRadar radar;
    radar.SetCenterPosition(100.0f, 100.0f);
    radar.SetRadius(50.0f);
    radar.SetScale(2.0f);
    
    float outX, outY;
    // Punkt 150, 100 - roznica: (150-100)/2 = 25, 0 (wewnatrz promienia 50)
    bool inside = radar.ProjectToRadar(150.0f, 100.0f, outX, outY);
    
    assert(inside == true);
    assert(std::abs(outX - 25.0f) < 0.001f);
    assert(std::abs(outY - 0.0f) < 0.001f);
    
    // Punkt 300, 100 - roznica: (300-100)/2 = 100, 0 (poza, przyciete do 50, 0)
    inside = radar.ProjectToRadar(300.0f, 100.0f, outX, outY);
    assert(inside == false);
    assert(std::abs(outX - 50.0f) < 0.001f);
    assert(std::abs(outY - 0.0f) < 0.001f);
}

void TestAddClearMarks() {
    MiniMapRadar radar;
    
    RadarMark mark1{10.0f, 20.0f, RadarMarkType::NPC, 1};
    RadarMark mark2{30.0f, 40.0f, RadarMarkType::Mob, 2};
    
    radar.AddMark(mark1);
    radar.AddMark(mark2);
    
    assert(radar.GetMarks().size() == 2);
    assert(radar.GetMarks()[0].id == 1);
    assert(radar.GetMarks()[1].id == 2);
    
    radar.ClearMarks();
    assert(radar.GetMarks().empty());
}

int main() {
    std::cout << "Starting MiniMapRadar Tests..." << std::endl;
    
    RUN_TEST(TestColors);
    RUN_TEST(TestProjectionInside);
    RUN_TEST(TestProjectionOutside);
    RUN_TEST(TestProjectionWithScale);
    RUN_TEST(TestAddClearMarks);
    
    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
