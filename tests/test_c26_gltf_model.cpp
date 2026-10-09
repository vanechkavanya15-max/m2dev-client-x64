#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>
#include "EterModelLib/GltfModel.h"
#include "EterModelLib/GltfModelInstance.h"

using namespace EterModelLib;

static bool ApproxEqual(float a, float b, float epsilon = 0.002f)
{
    return std::abs(a - b) < epsilon;
}

static bool ApproxEqualVec3(const float3& a, float x, float y, float z, float epsilon = 0.002f)
{
    return ApproxEqual(a.x, x, epsilon) &&
           ApproxEqual(a.y, y, epsilon) &&
           ApproxEqual(a.z, z, epsilon);
}

void TestModelAccessors()
{
    std::cout << "[RUN] TestModelAccessors..." << std::endl;

    CGltfModel model;
    assert(!model.IsLoaded());
    assert(model.GetSubmeshCount() == 0);
    assert(model.GetBoneCount() == 0);
    assert(model.GetAnimationCount() == 0);
    assert(model.GetVertexCount() == 0);

    // Wypelnienie reczne struktury m_modelData w celach testu jednostkowego
    GltfModelData& data = model.GetModelData();
    data.name = "TestModel";

    GltfSubmesh submesh1;
    submesh1.name = "Body";
    submesh1.materialIndex = 0;
    submesh1.vertexCount = 100;
    submesh1.indexCount = 300;
    data.submeshes.push_back(submesh1);

    GltfJoint jointRoot;
    jointRoot.name = "Root";
    jointRoot.parentIndex = -1;
    jointRoot.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    GltfJoint jointHand;
    jointHand.name = "Hand";
    jointHand.parentIndex = 0;
    jointHand.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,-10,0,1 };

    data.skin.joints.push_back(jointRoot);
    data.skin.joints.push_back(jointHand);

    GltfMotionData animAttack;
    animAttack.name = "attack_01";
    animAttack.duration = 1.0f;
    animAttack.events.push_back({ 0.35f, "ATTACK", "normal_hit" });
    animAttack.events.push_back({ 0.80f, "SOUND", "sword_swing.wav" });
    data.motions.push_back(animAttack);

    assert(model.GetName() == "TestModel");
    assert(model.GetSubmeshCount() == 1);
    assert(model.GetSubmesh(0)->name == "Body");
    assert(model.GetSubmesh(1) == nullptr);

    assert(model.GetBoneCount() == 2);
    assert(model.GetBone(0)->name == "Root");
    assert(model.GetBone(1)->name == "Hand");
    assert(model.GetBone(2) == nullptr);
    assert(model.FindBoneIndex("Root") == 0);
    assert(model.FindBoneIndex("Hand") == 1);
    assert(model.FindBoneIndex("NonExistent") == -1);

    assert(model.GetAnimationCount() == 1);
    assert(model.GetAnimation(0)->name == "attack_01");
    assert(model.GetAnimation(1) == nullptr);
    assert(model.FindAnimationIndex("attack_01") == 0);
    assert(model.FindAnimationIndex("non_existent") == -1);
    assert(model.FindAnimation("attack_01") != nullptr);
    assert(model.FindAnimation("attack_01")->events.size() == 2);

    std::cout << "[PASS] TestModelAccessors passed." << std::endl;
}

void TestInstanceAnimationAndHierarchy()
{
    std::cout << "[RUN] TestInstanceAnimationAndHierarchy..." << std::endl;

    CGltfModel model;
    GltfModelData& data = model.GetModelData();
    data.name = "AnimatedModel";

    // 2 kosci: 0 = Root, 1 = Weapon (dziecko Root)
    GltfJoint root;
    root.name = "Root";
    root.parentIndex = -1;
    root.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    GltfJoint child;
    child.name = "Weapon";
    child.parentIndex = 0;
    // inverse bind matrix przesuwa o -10 na osi X
    child.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, -10,0,0,1 };

    data.skin.joints.push_back(root);
    data.skin.joints.push_back(child);

    // Animacja z klatkami kluczowymi
    GltfMotionData motion;
    motion.name = "walk";
    motion.duration = 2.0f;

    // Kanal translacji Root: t=0 -> x=0, t=2 -> x=20
    GltfAnimationChannel chRootPos;
    chRootPos.jointIndex = 0;
    chRootPos.pathType = GltfAnimationPathType::Translation;
    chRootPos.times = { 0.0f, 2.0f };
    chRootPos.values = { {0.0f, 0.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f, 0.0f} };
    motion.channels.push_back(chRootPos);

    // Kanal translacji Weapon (lokalnie wzgledem Root): stala translacja x=10
    GltfAnimationChannel chChildPos;
    chChildPos.jointIndex = 1;
    chChildPos.pathType = GltfAnimationPathType::Translation;
    chChildPos.times = { 0.0f, 2.0f };
    chChildPos.values = { {10.0f, 0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f, 0.0f} };
    motion.channels.push_back(chChildPos);

    data.motions.push_back(motion);

    // ModelInstance
    CGltfModelInstance instance;
    instance.SetModel(&model);
    assert(instance.GetLocalTransforms().size() == 2);
    assert(instance.GetWorldTransforms().size() == 2);
    assert(instance.GetSkinningMatrices().size() == 2);

    bool setAnimOk = instance.SetAnimation("walk", true);
    assert(setAnimOk);
    assert(instance.GetCurrentAnimationIndex() == 0);
    assert(instance.GetDuration() == 2.0f);
    assert(instance.IsLoop() == true);

    // Update na t = 0.5s (w polowie 1/4 czasu animacji):
    // Root pos interpoluje do x = 5.0
    // Weapon lokalnie x = 10.0
    // Weapon swiatowo (Root * Child w hierarchii) = 5.0 + 10.0 = 15.0!
    instance.Update(0.5f);
    assert(ApproxEqual(instance.GetCurrentTime(), 0.5f));

    Matrix4x4 rootWorld;
    assert(instance.GetBoneWorldTransform(0, rootWorld));
    assert(ApproxEqual(rootWorld.m[3][0], 5.0f));

    Matrix4x4 weaponWorld;
    assert(instance.GetBoneWorldTransform(1, weaponWorld));
    assert(ApproxEqual(weaponWorld.m[3][0], 15.0f));

    // Macierz skinningu dla weapon: invBind (-10 na X) * world (15 na X) = 5 na X!
    const auto& skinMats = instance.GetSkinningMatrices();
    assert(ApproxEqual(skinMats[1].m[3][0], 5.0f));

    std::cout << "[PASS] TestInstanceAnimationAndHierarchy passed." << std::endl;
}

void TestCpuSkinningDeformation()
{
    std::cout << "[RUN] TestCpuSkinningDeformation..." << std::endl;

    CGltfModel model;
    GltfModelData& data = model.GetModelData();
    data.name = "SkinnedMesh";

    // 2 kosci
    GltfJoint bone0;
    bone0.name = "Bone0";
    bone0.parentIndex = -1;
    bone0.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    GltfJoint bone1;
    bone1.name = "Bone1";
    bone1.parentIndex = -1;
    bone1.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    data.skin.joints.push_back(bone0);
    data.skin.joints.push_back(bone1);

    // Wierzcholki
    // V0: 100% Bone0
    GltfVertex v0;
    v0.position = { 1.0f, 2.0f, 3.0f };
    v0.normal = { 0.0f, 1.0f, 0.0f };
    v0.jointIndices = { 0, 0, 0, 0 };
    v0.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

    // V1: 50% Bone0, 50% Bone1
    GltfVertex v1;
    v1.position = { 0.0f, 0.0f, 0.0f };
    v1.normal = { 0.0f, 0.0f, 1.0f };
    v1.jointIndices = { 0, 1, 0, 0 };
    v1.jointWeights = { 0.5f, 0.5f, 0.0f, 0.0f };

    // V2: brak wag (rigid)
    GltfVertex v2;
    v2.position = { 7.0f, 8.0f, 9.0f };
    v2.normal = { 1.0f, 0.0f, 0.0f };
    v2.jointIndices = { 0, 0, 0, 0 };
    v2.jointWeights = { 0.0f, 0.0f, 0.0f, 0.0f };

    data.vertices.push_back(v0);
    data.vertices.push_back(v1);
    data.vertices.push_back(v2);

    // Animacja przesuwajaca Bone0 o (10, 0, 0) a Bone1 o (0, 20, 0)
    GltfMotionData anim;
    anim.name = "move";
    anim.duration = 1.0f;

    GltfAnimationChannel ch0;
    ch0.jointIndex = 0;
    ch0.pathType = GltfAnimationPathType::Translation;
    ch0.times = { 0.0f, 1.0f };
    ch0.values = { {10.0f, 0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f, 0.0f} };
    anim.channels.push_back(ch0);

    GltfAnimationChannel ch1;
    ch1.jointIndex = 1;
    ch1.pathType = GltfAnimationPathType::Translation;
    ch1.times = { 0.0f, 1.0f };
    ch1.values = { {0.0f, 20.0f, 0.0f, 0.0f}, {0.0f, 20.0f, 0.0f, 0.0f} };
    anim.channels.push_back(ch1);

    data.motions.push_back(anim);

    CGltfModelInstance instance(&model);
    instance.SetAnimation(0, false);
    instance.Update(0.0f);
    instance.DeformVertices();

    const auto& defVerts = instance.GetDeformedVertices();
    assert(defVerts.size() == 3);

    // V0: z (1, 2, 3) przesuniete przez Bone0 (10, 0, 0) -> (11, 2, 3)
    assert(ApproxEqualVec3(defVerts[0].position, 11.0f, 2.0f, 3.0f));
    assert(ApproxEqualVec3(defVerts[0].normal, 0.0f, 1.0f, 0.0f));

    // V1: z (0, 0, 0) w 50% Bone0 (10,0,0) i 50% Bone1 (0,20,0) -> 0.5*(10,0,0) + 0.5*(0,20,0) = (5, 10, 0)
    assert(ApproxEqualVec3(defVerts[1].position, 5.0f, 10.0f, 0.0f));
    assert(ApproxEqualVec3(defVerts[1].normal, 0.0f, 0.0f, 1.0f));

    // V2: brak wag -> niezmienione (7, 8, 9)
    assert(ApproxEqualVec3(defVerts[2].position, 7.0f, 8.0f, 9.0f));
    assert(ApproxEqualVec3(defVerts[2].normal, 1.0f, 0.0f, 0.0f));

    std::cout << "[PASS] TestCpuSkinningDeformation passed." << std::endl;
}

void TestCombatEvents()
{
    std::cout << "[RUN] TestCombatEvents..." << std::endl;

    CGltfModel model;
    GltfModelData& data = model.GetModelData();
    data.name = "EventModel";

    GltfMotionData combo;
    combo.name = "combo_slash";
    combo.duration = 1.0f;
    combo.events.push_back({ 0.20f, "ATTACK", "hit1" });
    combo.events.push_back({ 0.50f, "SOUND", "whoosh.wav" });
    combo.events.push_back({ 0.70f, "ATTACK", "hit2" });
    combo.events.push_back({ 0.95f, "EFFECT", "sparks" });

    data.motions.push_back(combo);

    CGltfModelInstance instance(&model);
    instance.SetAnimation(0, true);

    // Krok 1: 0.0 -> 0.3s (powinien wyzwolic "hit1" na 0.20s)
    std::vector<GltfEvent> events1;
    instance.CheckEvents(0.0f, 0.3f, events1);
    assert(events1.size() == 1);
    assert(events1[0].type == "ATTACK");
    assert(events1[0].arg == "hit1");

    // Krok 2: 0.3 -> 0.8s (powinien wyzwolic SOUND na 0.5s i ATTACK na 0.7s)
    std::vector<GltfEvent> events2;
    instance.CheckEvents(0.3f, 0.8f, events2);
    assert(events2.size() == 2);
    assert(events2[0].type == "SOUND");
    assert(events2[1].type == "ATTACK");

    // Krok 3: Przeskok w petli z 0.90s do 0.10s (powinien trafic EFFECT na 0.95s)
    std::vector<GltfEvent> eventsLoop;
    instance.CheckEvents(0.90f, 0.10f, eventsLoop);
    assert(eventsLoop.size() == 1);
    assert(eventsLoop[0].type == "EFFECT");
    assert(eventsLoop[0].arg == "sparks");

    std::cout << "[PASS] TestCombatEvents passed." << std::endl;
}

void TestLoaderErrorHandling()
{
    std::cout << "[RUN] TestLoaderErrorHandling..." << std::endl;

    CGltfModel model;
    assert(!model.LoadFromFile("non_existent_model_file_12345.gltf"));
    assert(!model.IsLoaded());

    const char invalidData[] = "invalid_binary_data";
    assert(!model.LoadFromMemory(invalidData, sizeof(invalidData)));
    assert(!model.IsLoaded());

    std::cout << "[PASS] TestLoaderErrorHandling passed." << std::endl;
}

int main()
{
    std::cout << "--- START TEST_C26_GLTF_MODEL ---" << std::endl;
    TestModelAccessors();
    TestInstanceAnimationAndHierarchy();
    TestCpuSkinningDeformation();
    TestCombatEvents();
    TestLoaderErrorHandling();
    std::cout << "--- ALL GLTF MODEL TESTS PASSED SUCCESSFULLY! ---" << std::endl;
    return 0;
}
