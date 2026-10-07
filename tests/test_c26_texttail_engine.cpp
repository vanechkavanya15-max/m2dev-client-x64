#include "../src/Client/UI/TextTailEngine.h"
#include "../src/EterBase/ModernLogger.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace UserInterface::TextTail;

// Lightweight test runner as required
int main() {
    std::cout << "Running TextTailEngine Tests..." << std::endl;
    
    TextTailEngine engine;
    EterBase::EntityId vid1{1001};
    EterBase::EntityId vid2{1002};

    // Test Registration
    auto res1 = engine.RegisterActor(vid1, "Hero", 0.5f);
    assert(res1.has_value());

    auto res2 = engine.RegisterActor(vid2, "Monster", 0.2f);
    assert(res2.has_value());

    // Test Guild, Title, Karma
    engine.SetActorGuild(vid1, "Knights");
    engine.SetActorTitle(vid1, "Sir");
    engine.SetActorKarma(vid1, AlignmentKarma::Chivalric);

    auto heroOpt = engine.GetActorTextTail(vid1);
    assert(heroOpt.has_value());
    assert(heroOpt->guildName == "Knights");

    // Test Items
    engine.RegisterItem(500, "Sword", {10.0f, 10.0f, 0.0f});
    auto itemOpt = engine.GetItemTextTail(500);
    assert(itemOpt.has_value());
    assert(itemOpt->itemName == "Sword");

    // Test Projection Math (Mock Identity-ish Matrices)
    CameraState cam;
    cam.screenWidth = 800;
    cam.screenHeight = 600;
    cam.position = {0.0f, 0.0f, -5.0f}; // Camera is back along Z
    
    for (int i=0; i<16; ++i) cam.viewMatrix[i] = (i%5==0) ? 1.0f : 0.0f;
    for (int i=0; i<16; ++i) cam.projMatrix[i] = (i%5==0) ? 1.0f : 0.0f;
    cam.projMatrix[15] = 1.0f;
    cam.viewMatrix[15] = 1.0f;

    Vector3 pos1{0.0f, 0.0f, 0.0f};
    engine.UpdateProjection(vid1, pos1, cam);
    
    heroOpt = engine.GetActorTextTail(vid1);
    assert(heroOpt->isVisible == true);
    
    // Test Distance using camera pos
    // Actor pos is {0, 0, 0}, camera pos is {0, 0, -5}. Distance should be 5.0.
    assert(std::abs(heroOpt->distance - 5.0f) < 0.1f);

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
