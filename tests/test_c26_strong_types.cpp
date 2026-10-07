#include "../src/Client/Core/StrongTypes.h"
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <cmath>

using namespace Client::Core;

void TestStrongTypes() {
    EntityVid vid1{100};
    EntityVid vid2{100};
    EntityVid vid3{200};
    
    // Test get()
    assert(vid1.get() == 100);
    assert(vid3.get() == 200);
    
    // Test comparison
    assert(vid1 == vid2);
    assert(vid1 != vid3);
    assert(vid1 < vid3);
    assert(vid3 > vid1);
    
    // Test default value
    EntityVid defVid;
    assert(defVid.get() == 0);
    
    ItemSlot slot;
    assert(slot.get() == 0xFFFF);
    
    // Test hashing
    std::unordered_map<EntityVid, int> vidMap;
    vidMap[vid1] = 42;
    assert(vidMap[vid1] == 42);
    assert(vidMap[vid2] == 42);
    
    // Test formatting
    std::string formatted = std::format("{}", vid1);
    assert(formatted == "100");
}

void TestMapCoords() {
    MapCoords c1{1.0f, 2.0f, 3.0f};
    MapCoords c2{4.0f, 5.0f, 6.0f};
    
    // Test default constructor
    MapCoords def;
    assert(def.x == 0.0f && def.y == 0.0f && def.z == 0.0f);
    
    // Test arithmetic
    MapCoords sum = c1 + c2;
    assert(sum.x == 5.0f && sum.y == 7.0f && sum.z == 9.0f);
    
    MapCoords diff = c2 - c1;
    assert(diff.x == 3.0f && diff.y == 3.0f && diff.z == 3.0f);
    
    MapCoords scaled = c1 * 2.0f;
    assert(scaled.x == 2.0f && scaled.y == 4.0f && scaled.z == 6.0f);
    
    MapCoords divided = c2 / 2.0f;
    assert(divided.x == 2.0f && divided.y == 2.5f && divided.z == 3.0f);
    
    // Test assignment arithmetic
    MapCoords temp = c1;
    temp += c2;
    assert(temp == sum);
    
    // Test comparison
    assert(c1 == c1);
    assert(c1 != c2);
    
    // Test vector math
    float dot = c1.Dot(c2);
    assert(std::abs(dot - (1*4 + 2*5 + 3*6)) < 0.001f);
    
    MapCoords cross = c1.Cross(c2);
    assert(cross.x == -3.0f && cross.y == 6.0f && cross.z == -3.0f);
    
    MapCoords c3{3.0f, 4.0f, 0.0f};
    assert(std::abs(c3.Length() - 5.0f) < 0.001f);
    
    MapCoords c4{0.0f, 0.0f, 0.0f};
    assert(std::abs(c3.Distance(c4) - 5.0f) < 0.001f);
    
    MapCoords norm = c3.Normalize();
    assert(std::abs(norm.x - 0.6f) < 0.001f);
    assert(std::abs(norm.y - 0.8f) < 0.001f);
    assert(std::abs(norm.z - 0.0f) < 0.001f);
    assert(std::abs(norm.Length() - 1.0f) < 0.001f);
    
    // Test zero normalize
    MapCoords zero;
    MapCoords zeroNorm = zero.Normalize();
    assert(zeroNorm.x == 0.0f && zeroNorm.y == 0.0f && zeroNorm.z == 0.0f);
    
    // Test hashing
    std::unordered_map<MapCoords, int> mapMap;
    mapMap[c1] = 99;
    assert(mapMap[c1] == 99);
    
    // Test formatting
    std::string formatted = std::format("{}", c1);
    assert(formatted == "(1, 2, 3)");
}

int main() {
    std::cout << "Running StrongTypes tests...\n";
    TestStrongTypes();
    std::cout << "StrongTypes tests passed.\n";
    
    std::cout << "Running MapCoords tests...\n";
    TestMapCoords();
    std::cout << "MapCoords tests passed.\n";
    
    std::cout << "All tests passed successfully.\n";
    return 0;
}
