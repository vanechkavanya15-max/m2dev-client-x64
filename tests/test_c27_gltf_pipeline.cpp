#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

#include "EterModelLib/GltfModel.h"
#include "EterModelLib/GltfModelInstance.h"
#include "EterModelLib/GltfLoader.h"
#include "EterModelLib/SkeletalMath.h"

using namespace EterModelLib;

static bool ApproxEqual(float a, float b, float epsilon = 0.005f)
{
    return std::abs(a - b) < epsilon;
}

static bool ApproxEqualVec3(const float3& a, float x, float y, float z, float epsilon = 0.005f)
{
    return ApproxEqual(a.x, x, epsilon) &&
           ApproxEqual(a.y, y, epsilon) &&
           ApproxEqual(a.z, z, epsilon);
}

// ------------------------------------------------------------------------------------------------
// Krok 1: Test pelnej inicjalizacji modelu danymi szkieletu, podsiatek, wag i animacji
// ------------------------------------------------------------------------------------------------
void TestPipelineInitialization(CGltfModel& outModel)
{
    std::cout << "[RUN] TestPipelineInitialization..." << std::endl;

    GltfModelData& data = outModel.GetModelData();
    data.name = "WarriorAttackModel";

    // Submesh
    GltfSubmesh mesh;
    mesh.name = "WarriorMesh";
    mesh.materialIndex = 0;
    mesh.vertexOffset = 0;
    mesh.vertexCount = 4;
    mesh.indexOffset = 0;
    mesh.indexCount = 6;
    data.submeshes.push_back(mesh);

    // Indeksy (2 trojkaty)
    data.indices = { 0, 1, 2, 0, 2, 3 };

    // Szkielet:
    // Kosz 0: "Root" (brak rodzica)
    GltfJoint jointRoot;
    jointRoot.name = "Root";
    jointRoot.parentIndex = -1;
    jointRoot.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    // Kosc 1: "Bone_Arm" (dziecko kosci Root)
    GltfJoint jointArm;
    jointArm.name = "Bone_Arm";
    jointArm.parentIndex = 0;
    jointArm.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    data.skin.name = "WarriorSkin";
    data.skin.joints.push_back(jointRoot);
    data.skin.joints.push_back(jointArm);

    // Wierzcholki:
    // V0: Przyczepiony 100% do obracanej kosci Bone_Arm (indeks 1)
    GltfVertex v0;
    v0.position = { 10.0f, 0.0f, 0.0f };
    v0.normal = { 1.0f, 0.0f, 0.0f };
    v0.uv0 = { 0.0f, 0.0f };
    v0.uv1 = { 0.0f, 0.0f };
    v0.jointIndices = { 1, 0, 0, 0 };
    v0.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

    // V1: Przyczepiony 100% do kosci Root (indeks 0, brak animacji)
    GltfVertex v1;
    v1.position = { 0.0f, 5.0f, 0.0f };
    v1.normal = { 0.0f, 1.0f, 0.0f };
    v1.uv0 = { 0.5f, 0.0f };
    v1.uv1 = { 0.5f, 0.0f };
    v1.jointIndices = { 0, 0, 0, 0 };
    v1.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

    // V2: Wagi mieszane: 50% Bone_Arm, 50% Root
    GltfVertex v2;
    v2.position = { 10.0f, 0.0f, 0.0f };
    v2.normal = { 1.0f, 0.0f, 0.0f };
    v2.uv0 = { 1.0f, 1.0f };
    v2.uv1 = { 1.0f, 1.0f };
    v2.jointIndices = { 1, 0, 0, 0 };
    v2.jointWeights = { 0.5f, 0.5f, 0.0f, 0.0f };

    // V3: Sztywny wierzcholek (brak wag)
    GltfVertex v3;
    v3.position = { 2.0f, 3.0f, 4.0f };
    v3.normal = { 0.0f, 0.0f, 1.0f };
    v3.uv0 = { 0.0f, 1.0f };
    v3.uv1 = { 0.0f, 1.0f };
    v3.jointIndices = { 0, 0, 0, 0 };
    v3.jointWeights = { 0.0f, 0.0f, 0.0f, 0.0f };

    data.vertices.push_back(v0);
    data.vertices.push_back(v1);
    data.vertices.push_back(v2);
    data.vertices.push_back(v3);

    // Animacja "attack_combo"
    // Czas trwania: 1.0 sekundy
    // Rotacja kosci Bone_Arm kwaternionem wokol osi Z:
    //  t = 0.0s: kat 0 st. -> Quat(0, 0, 0, 1)
    //  t = 0.5s: kat 90 st. wokol Z -> Quat(0, 0, sin(45 st.), cos(45 st.)) = (0, 0, 0.7071068, 0.7071068)
    //  t = 1.0s: kat 180 st. wokol Z -> Quat(0, 0, sin(90 st.), cos(90 st.)) = (0, 0, 1.0, 0.0)
    GltfMotionData anim;
    anim.name = "attack_combo";
    anim.duration = 1.0f;

    GltfAnimationChannel chRotArm;
    chRotArm.jointIndex = 1;
    chRotArm.pathType = GltfAnimationPathType::Rotation;
    chRotArm.times = { 0.0f, 0.5f, 1.0f };
    chRotArm.values = {
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 0.7071068f, 0.7071068f },
        { 0.0f, 0.0f, 1.0f, 0.0f }
    };
    anim.channels.push_back(chRotArm);

    // Eventy Metin2:
    //  t = 0.20s: SOUND (swing)
    //  t = 0.45s: ATTACK (hit_slash)
    anim.events.push_back({ 0.20f, "SOUND", "sound/pc/warrior/sword_swing.wav" });
    anim.events.push_back({ 0.45f, "ATTACK", "hit_slash" });

    data.motions.push_back(anim);

    // Weryfikacja poprawnosci modelu
    assert(outModel.GetName() == "WarriorAttackModel");
    assert(outModel.GetSubmeshCount() == 1);
    assert(outModel.GetBoneCount() == 2);
    assert(outModel.FindBoneIndex("Root") == 0);
    assert(outModel.FindBoneIndex("Bone_Arm") == 1);
    assert(outModel.GetVertexCount() == 4);
    assert(outModel.GetIndexCount() == 6);
    assert(outModel.GetAnimationCount() == 1);
    assert(outModel.FindAnimationIndex("attack_combo") == 0);

    std::cout << "[PASS] TestPipelineInitialization passed." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// Krok 2: Test CGltfModelInstance - PlayMotion, Update(dt), macierze skinningu
// ------------------------------------------------------------------------------------------------
void TestPlayMotionAndUpdate(CGltfModel& model)
{
    std::cout << "[RUN] TestPlayMotionAndUpdate..." << std::endl;

    CGltfModelInstance instance(&model);
    assert(instance.GetModel() == &model);
    assert(instance.GetLocalTransforms().size() == 2);
    assert(instance.GetWorldTransforms().size() == 2);
    assert(instance.GetSkinningMatrices().size() == 2);

    // Uruchomienie animacji za pomoca metody PlayMotion
    bool playOk = instance.PlayMotion("attack_combo", false);
    assert(playOk);
    assert(instance.GetCurrentAnimationIndex() == 0);
    assert(ApproxEqual(instance.GetDuration(), 1.0f));
    assert(instance.IsLoop() == false);
    assert(ApproxEqual(instance.GetCurrentTime(), 0.0f));

    // Stan poczatkowy t = 0.0s: brak rotacji
    const auto& initSkinMats = instance.GetSkinningMatrices();
    assert(ApproxEqual(initSkinMats[1].m[0][0], 1.0f));
    assert(ApproxEqual(initSkinMats[1].m[1][1], 1.0f));
    assert(ApproxEqual(initSkinMats[1].m[2][2], 1.0f));

    // Wywolanie Update(0.5f) -> t = 0.5s: rotacja o 90 stopni wokol Z
    instance.Update(0.5f);
    assert(ApproxEqual(instance.GetCurrentTime(), 0.5f));

    const auto& skinMats90 = instance.GetSkinningMatrices();
    // Macierz rotacji o 90 st. w leworecznym ukladzie wokol Z:
    // m[0][0] = 0, m[0][1] = -1, m[1][0] = 1, m[1][1] = 0
    assert(ApproxEqual(skinMats90[1].m[0][0], 0.0f));
    assert(ApproxEqual(skinMats90[1].m[0][1], -1.0f));
    assert(ApproxEqual(skinMats90[1].m[1][0], 1.0f));
    assert(ApproxEqual(skinMats90[1].m[1][1], 0.0f));
    assert(ApproxEqual(skinMats90[1].m[2][2], 1.0f));

    // Odczyt macierzy swiatowej kosci (GetBoneWorldTransform)
    Matrix4x4 armWorldTransform;
    bool boneFound = instance.GetBoneWorldTransform("Bone_Arm", armWorldTransform);
    assert(boneFound);
    assert(ApproxEqual(armWorldTransform.m[0][0], 0.0f));
    assert(ApproxEqual(armWorldTransform.m[0][1], -1.0f));

    std::cout << "[PASS] TestPlayMotionAndUpdate passed." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// Krok 3: Sprawdzenie metody DeformVertices() - rotacja wierzcholkow na CPU
// ------------------------------------------------------------------------------------------------
void TestDeformVertices(CGltfModel& model)
{
    std::cout << "[RUN] TestDeformVertices..." << std::endl;

    CGltfModelInstance instance(&model);
    instance.PlayMotion("attack_combo", false);

    // Krok 3A: Aktualizacja do t = 0.5s (rotacja 90 stopni)
    instance.Update(0.5f);
    instance.DeformVertices();

    const auto& defVerts90 = instance.GetDeformedVertices();
    assert(defVerts90.size() == 4);

    // V0: poczatkowo (10.0, 0.0, 0.0) z waga 1.0 do Bone_Arm.
    // Po rotacji o 90 stopni wokol Z:
    // [10, 0, 0, 1] * M -> x = 10 * 0 = 0, y = 10 * (-1) = -10, z = 0.
    assert(ApproxEqualVec3(defVerts90[0].position, 0.0f, -10.0f, 0.0f));
    // Normalna poczatkowa (1.0, 0.0, 0.0) -> (0.0, -1.0, 0.0)
    assert(ApproxEqualVec3(defVerts90[0].normal, 0.0f, -1.0f, 0.0f));

    // V1: poczatkowo (0.0, 5.0, 0.0) z waga 1.0 do Root (Root nie ma rotacji) -> pozostaje bez zmian
    assert(ApproxEqualVec3(defVerts90[1].position, 0.0f, 5.0f, 0.0f));
    assert(ApproxEqualVec3(defVerts90[1].normal, 0.0f, 1.0f, 0.0f));

    // V2: poczatkowo (10.0, 0.0, 0.0) z waga 50% Bone_Arm i 50% Root:
    // Wplyw Bone_Arm (waga 0.5) -> 0.5 * (0, -10, 0) = (0, -5, 0)
    // Wplyw Root (waga 0.5)     -> 0.5 * (10, 0, 0)  = (5, 0, 0)
    // Pozycja wynikowa: (5.0, -5.0, 0.0)
    assert(ApproxEqualVec3(defVerts90[2].position, 5.0f, -5.0f, 0.0f));

    // V3: wierzcholek bez wag (sztywny) -> (2, 3, 4)
    assert(ApproxEqualVec3(defVerts90[3].position, 2.0f, 3.0f, 4.0f));

    // Krok 3B: Aktualizacja do t = 1.0s (kolejne dt = 0.5s -> rotacja 180 stopni wokol Z)
    instance.Update(0.5f);
    assert(ApproxEqual(instance.GetCurrentTime(), 1.0f));
    instance.DeformVertices();

    const auto& defVerts180 = instance.GetDeformedVertices();
    // V0: poczatkowo (10.0, 0.0, 0.0) obrocone o 180 stopni wokol Z -> (-10.0, 0.0, 0.0)
    assert(ApproxEqualVec3(defVerts180[0].position, -10.0f, 0.0f, 0.0f));
    assert(ApproxEqualVec3(defVerts180[0].normal, -1.0f, 0.0f, 0.0f));

    std::cout << "[PASS] TestDeformVertices passed." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// Krok 4: Sprawdzenie metody CheckEvents() - precyzyjne odpalanie eventu ATTACK
// ------------------------------------------------------------------------------------------------
void TestCheckEvents(CGltfModel& model)
{
    std::cout << "[RUN] TestCheckEvents..." << std::endl;

    CGltfModelInstance instance(&model);
    instance.PlayMotion("attack_combo", false);

    // Krok 4A: Klatka 0.0s do 0.15s - zaden event nie powinien sie odpalic
    std::vector<GltfEvent> eventsPhase1;
    instance.CheckEvents(0.0f, 0.15f, eventsPhase1);
    assert(eventsPhase1.empty());

    // Krok 4B: Klatka 0.15s do 0.30s - powinnismy trafic na event "SOUND" (t = 0.20s)
    std::vector<GltfEvent> eventsPhase2;
    instance.CheckEvents(0.15f, 0.30f, eventsPhase2);
    assert(eventsPhase2.size() == 1);
    assert(eventsPhase2[0].type == "SOUND");
    assert(eventsPhase2[0].arg == "sound/pc/warrior/sword_swing.wav");
    assert(ApproxEqual(eventsPhase2[0].time, 0.20f));

    // Krok 4C: Klatka 0.30s do 0.44s - tuz przed eventem ATTACK
    std::vector<GltfEvent> eventsPhase3;
    instance.CheckEvents(0.30f, 0.44f, eventsPhase3);
    assert(eventsPhase3.empty());

    // Krok 4D: Klatka 0.44s do 0.50s - w tym oknie odpala sie dokladnie event "ATTACK" (t = 0.45s)
    std::vector<GltfEvent> eventsPhase4;
    instance.CheckEvents(0.44f, 0.50f, eventsPhase4);
    assert(eventsPhase4.size() == 1);
    assert(eventsPhase4[0].type == "ATTACK");
    assert(eventsPhase4[0].arg == "hit_slash");
    assert(ApproxEqual(eventsPhase4[0].time, 0.45f));

    // Krok 4E: Klatka 0.50s do 1.00s - brak dalszych eventow
    std::vector<GltfEvent> eventsPhase5;
    instance.CheckEvents(0.50f, 1.00f, eventsPhase5);
    assert(eventsPhase5.empty());

    // Krok 4F: Test przez Update(dt) i przeciazenie CheckEvents(outEvents)
    CGltfModelInstance instanceAuto(&model);
    instanceAuto.PlayMotion("attack_combo", false);

    std::vector<GltfEvent> autoEvents;
    instanceAuto.Update(0.10f); // 0.0 -> 0.1s
    instanceAuto.CheckEvents(autoEvents);
    assert(autoEvents.empty());

    instanceAuto.Update(0.15f); // 0.1 -> 0.25s (zawiera SOUND na 0.20s)
    instanceAuto.CheckEvents(autoEvents);
    assert(autoEvents.size() == 1);
    assert(autoEvents[0].type == "SOUND");
    autoEvents.clear();

    instanceAuto.Update(0.15f); // 0.25 -> 0.40s
    instanceAuto.CheckEvents(autoEvents);
    assert(autoEvents.empty());

    instanceAuto.Update(0.10f); // 0.40 -> 0.50s (zawiera ATTACK na 0.45s)
    instanceAuto.CheckEvents(autoEvents);
    assert(autoEvents.size() == 1);
    assert(autoEvents[0].type == "ATTACK");
    assert(autoEvents[0].arg == "hit_slash");
    autoEvents.clear();

    std::cout << "[PASS] TestCheckEvents passed." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// Krok 5: Test GltfLoader - wczytywanie JSON glTF z materialami (tekstury) i metin2_events (walka/dzwiek)
// ------------------------------------------------------------------------------------------------
void TestGltfLoaderEventsAndMaterialsExtraction()
{
    std::cout << "[RUN] TestGltfLoaderEventsAndMaterialsExtraction..." << std::endl;

    std::string gltfJson = 
        "{\n"
        "  \"asset\": { \"version\": \"2.0\" },\n"
        "  \"materials\": [\n"
        "    {\n"
        "      \"name\": \"WarriorBodyMaterial\",\n"
        "      \"extras\": { \"diffuse_texture\": \"d:/ymir work/pc/warrior/warrior_m_body.dds\", \"opacity_texture\": \"\" }\n"
        "    }\n"
        "  ],\n"
        "  \"nodes\": [\n"
        "    { \"name\": \"Root\" }\n"
        "  ],\n"
        "  \"animations\": [\n"
        "    {\n"
        "      \"name\": \"combo_attack_01\",\n"
        "      \"extras\": {\n"
        "        \"metin2_events\": [\n"
        "          { \"time\": 0.25, \"type\": \"ATTACK\" },\n"
        "          { \"time\": 0.60, \"type\": \"SOUND\" }\n"
        "        ]\n"
        "      },\n"
        "      \"samplers\": [],\n"
        "      \"channels\": []\n"
        "    }\n"
        "  ]\n"
        "}";

    GltfLoader loader;
    GltfModelData modelData;
    bool ok = loader.LoadFromMemory(gltfJson.data(), gltfJson.size(), modelData);
    assert(ok && "GltfLoader::LoadFromMemory should succeed");

    // Weryfikacja materialu i sciezki tekstury
    assert(modelData.materials.size() == 1);
    assert(modelData.materials[0].name == "WarriorBodyMaterial");
    assert(modelData.materials[0].diffuseTexture == "d:/ymir work/pc/warrior/warrior_m_body.dds");

    // Weryfikacja animacji i metin2_events (ATTACK i SOUND)
    assert(modelData.motions.size() == 1);
    assert(modelData.motions[0].name == "combo_attack_01");
    assert(modelData.motions[0].events.size() == 2);

    assert(modelData.motions[0].events[0].type == "ATTACK");
    assert(ApproxEqual(modelData.motions[0].events[0].time, 0.25f));

    assert(modelData.motions[0].events[1].type == "SOUND");
    assert(ApproxEqual(modelData.motions[0].events[1].time, 0.60f));

    std::cout << "[PASS] TestGltfLoaderEventsAndMaterialsExtraction passed." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// Krok 6: Test gniazd kosci (Sockets/Bone Attachment) oraz laczenia szkieletow (Hair Link)
// ------------------------------------------------------------------------------------------------
void TestBoneAttachmentAndSkeletonLink()
{
    std::cout << "[RUN] TestBoneAttachmentAndSkeletonLink..." << std::endl;

    // 1. Tworzymy glowny model postaci (Body) ze szkieletem:
    // Root -> Spine -> Head
    //               -> R Hand
    CGltfModel bodyModel;
    GltfModelData& bodyData = bodyModel.GetModelData();
    bodyData.name = "BodyModel";

    GltfJoint jRoot;
    jRoot.name = "Root";
    jRoot.parentIndex = -1;
    jRoot.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    GltfJoint jSpine;
    jSpine.name = "Bip01 Spine";
    jSpine.parentIndex = 0;
    jSpine.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    GltfJoint jHead;
    jHead.name = "Bip01 Head";
    jHead.parentIndex = 1;
    jHead.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    GltfJoint jHand;
    jHand.name = "Bip01 R Hand";
    jHand.parentIndex = 1;
    jHand.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    bodyData.skin.name = "BodySkin";
    bodyData.skin.joints.push_back(jRoot);
    bodyData.skin.joints.push_back(jSpine);
    bodyData.skin.joints.push_back(jHead);
    bodyData.skin.joints.push_back(jHand);

    // Animacja bazowa ustawiajaca kosci w odpowiednich pozycjach
    GltfMotionData bodyMotion;
    bodyMotion.name = "body_bind";
    bodyMotion.duration = 1.0f;

    // Root w (0, 0, 0)
    GltfAnimationChannel chRoot;
    chRoot.jointIndex = 0;
    chRoot.pathType = GltfAnimationPathType::Translation;
    chRoot.times = { 0.0f };
    chRoot.values = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    bodyMotion.channels.push_back(chRoot);

    // Spine: translacja (0, 10, 0) wzgledem Root -> world (0, 10, 0)
    GltfAnimationChannel chSpine;
    chSpine.jointIndex = 1;
    chSpine.pathType = GltfAnimationPathType::Translation;
    chSpine.times = { 0.0f };
    chSpine.values = { { 0.0f, 10.0f, 0.0f, 0.0f } };
    bodyMotion.channels.push_back(chSpine);

    // Head: translacja (0, 10, 0) wzgledem Spine -> world (0, 20, 0)
    GltfAnimationChannel chHead;
    chHead.jointIndex = 2;
    chHead.pathType = GltfAnimationPathType::Translation;
    chHead.times = { 0.0f };
    chHead.values = { { 0.0f, 10.0f, 0.0f, 0.0f } };
    bodyMotion.channels.push_back(chHead);

    // R Hand: translacja (15, 0, 0) wzgledem Spine -> world (15, 10, 0)
    GltfAnimationChannel chHand;
    chHand.jointIndex = 3;
    chHand.pathType = GltfAnimationPathType::Translation;
    chHand.times = { 0.0f };
    chHand.values = { { 15.0f, 0.0f, 0.0f, 0.0f } };
    bodyMotion.channels.push_back(chHand);

    bodyData.motions.push_back(bodyMotion);

    // Wierzcholki ciala (np. glowa w (0, 20, 0))
    GltfVertex vHead;
    vHead.position = { 0.0f, 20.0f, 0.0f };
    vHead.normal = { 0.0f, 1.0f, 0.0f };
    vHead.uv0 = { 0.0f, 0.0f };
    vHead.uv1 = { 0.0f, 0.0f };
    vHead.jointIndices = { 2, 0, 0, 0 };
    vHead.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };
    bodyData.vertices.push_back(vHead);

    GltfSubmesh bodySubmesh;
    bodySubmesh.name = "BodySubmesh";
    bodySubmesh.vertexOffset = 0;
    bodySubmesh.vertexCount = 1;
    bodySubmesh.indexOffset = 0;
    bodySubmesh.indexCount = 0;
    bodyData.submeshes.push_back(bodySubmesh);

    bodyModel.SetLoaded(true);

    CGltfModelInstance bodyInstance(&bodyModel);
    bodyInstance.PlayMotion("body_bind", false);
    bodyInstance.Update(0.0f);

    // Sprawdzenie pobierania macierzy kosci GetBoneMatrixPointer(int) oraz GetBoneMatrixPointer(const char*)
    const Matrix4x4* pMatHandByIdx = bodyInstance.GetBoneMatrixPointer(3);
    assert(pMatHandByIdx != nullptr && "GetBoneMatrixPointer(3) musi zwrocic wskaznik");
    assert(ApproxEqual(pMatHandByIdx->m[3][0], 15.0f));
    assert(ApproxEqual(pMatHandByIdx->m[3][1], 10.0f));
    assert(ApproxEqual(pMatHandByIdx->m[3][2], 0.0f));

    const Matrix4x4* pMatHandByName = bodyInstance.GetBoneMatrixPointer("Bip01 R Hand");
    assert(pMatHandByName != nullptr && "GetBoneMatrixPointer('Bip01 R Hand') musi zwrocic wskaznik");
    assert(pMatHandByName == pMatHandByIdx);

    const Matrix4x4* pMatHead = bodyInstance.GetBoneMatrixPointer("Bip01 Head");
    assert(pMatHead != nullptr);
    assert(ApproxEqual(pMatHead->m[3][0], 0.0f));
    assert(ApproxEqual(pMatHead->m[3][1], 20.0f));
    assert(ApproxEqual(pMatHead->m[3][2], 0.0f));

    // Bledne zapytania
    assert(bodyInstance.GetBoneMatrixPointer(999) == nullptr);
    assert(bodyInstance.GetBoneMatrixPointer("NonExistentBone") == nullptr);

    // 2. Test gniazda broni (Weapon Socket Attachment) - sztywny miecz podpiety do dloni
    CGltfModel weaponModel;
    GltfModelData& weaponData = weaponModel.GetModelData();
    weaponData.name = "SwordModel";

    GltfVertex vHilt; // Rekojesc w (0, 0, 0)
    vHilt.position = { 0.0f, 0.0f, 0.0f };
    vHilt.normal = { 0.0f, 1.0f, 0.0f };
    vHilt.uv0 = { 0.0f, 0.0f };
    vHilt.uv1 = { 0.0f, 0.0f };
    weaponData.vertices.push_back(vHilt);

    GltfVertex vBlade; // Czubek miecza w (0, 50, 0)
    vBlade.position = { 0.0f, 50.0f, 0.0f };
    vBlade.normal = { 0.0f, 1.0f, 0.0f };
    vBlade.uv0 = { 0.0f, 1.0f };
    vBlade.uv1 = { 0.0f, 1.0f };
    weaponData.vertices.push_back(vBlade);

    GltfSubmesh weaponSubmesh;
    weaponSubmesh.name = "SwordSubmesh";
    weaponSubmesh.vertexOffset = 0;
    weaponSubmesh.vertexCount = 2;
    weaponSubmesh.indexOffset = 0;
    weaponSubmesh.indexCount = 0;
    weaponData.submeshes.push_back(weaponSubmesh);

    weaponModel.SetLoaded(true);

    CGltfModelInstance weaponInstance(&weaponModel);
    weaponInstance.SetParentModelInstance(&bodyInstance, "Bip01 R Hand");

    const Matrix4x4* pParentMat = weaponInstance.GetParentBoneMatrix();
    assert(pParentMat != nullptr && "GetParentBoneMatrix() musi zwrocic macierz rodzica");
    assert(ApproxEqual(pParentMat->m[3][0], 15.0f));
    assert(ApproxEqual(pParentMat->m[3][1], 10.0f));

    weaponInstance.Update(0.0f);
    weaponInstance.DeformVertices();

    const auto& defWeaponVerts = weaponInstance.GetDeformedVertices();
    assert(defWeaponVerts.size() == 2);
    // Rekojesc powinna znalezc sie dokladnie w pozycji dloni (15, 10, 0)
    assert(ApproxEqualVec3(defWeaponVerts[0].position, 15.0f, 10.0f, 0.0f));
    // Czubek miecza (0, 50, 0) + dlon (15, 10, 0) -> (15, 60, 0)
    assert(ApproxEqualVec3(defWeaponVerts[1].position, 15.0f, 60.0f, 0.0f));

    // 3. Test laczenia szkieletow (Hair Link)
    CGltfModel hairModel;
    GltfModelData& hairData = hairModel.GetModelData();
    hairData.name = "HairModel";

    GltfJoint jHairHead;
    jHairHead.name = "Bip01 Head";
    jHairHead.parentIndex = -1;
    jHairHead.inverseBindMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    hairData.skin.name = "HairSkin";
    hairData.skin.joints.push_back(jHairHead);

    GltfVertex vHair; // Wierzcholek wlosow (0, 2, 0) wzgledem glowy
    vHair.position = { 0.0f, 2.0f, 0.0f };
    vHair.normal = { 0.0f, 1.0f, 0.0f };
    vHair.uv0 = { 0.0f, 0.0f };
    vHair.uv1 = { 0.0f, 0.0f };
    vHair.jointIndices = { 0, 0, 0, 0 };
    vHair.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };
    hairData.vertices.push_back(vHair);

    GltfSubmesh hairSubmesh;
    hairSubmesh.name = "HairSubmesh";
    hairSubmesh.vertexOffset = 0;
    hairSubmesh.vertexCount = 1;
    hairSubmesh.indexOffset = 0;
    hairSubmesh.indexCount = 0;
    hairData.submeshes.push_back(hairSubmesh);

    hairModel.SetLoaded(true);

    CGltfModelInstance hairInstance(&hairModel);
    hairInstance.LinkSkeleton(&bodyInstance);
    assert(hairInstance.GetLinkedModelInstance() == &bodyInstance);

    hairInstance.Update(0.0f);
    hairInstance.DeformVertices();

    // Sprawdzamy, czy kosc wlosow otrzymala macierz ze szkieletu rodzica (0, 20, 0)
    const Matrix4x4* pHairHeadMat = hairInstance.GetBoneMatrixPointer("Bip01 Head");
    assert(pHairHeadMat != nullptr);
    assert(ApproxEqual(pHairHeadMat->m[3][0], 0.0f));
    assert(ApproxEqual(pHairHeadMat->m[3][1], 20.0f));
    assert(ApproxEqual(pHairHeadMat->m[3][2], 0.0f));

    // Pozycja wierzcholka wlosow: bind pose (0, 2, 0) - inverseBind (0,0,0) + head (0, 20, 0) -> (0, 22, 0)
    const auto& defHairVerts = hairInstance.GetDeformedVertices();
    assert(defHairVerts.size() == 1);
    assert(ApproxEqualVec3(defHairVerts[0].position, 0.0f, 22.0f, 0.0f));

    std::cout << "[PASS] TestBoneAttachmentAndSkeletonLink passed." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// Glowny punkt wejscia testu
// ------------------------------------------------------------------------------------------------
int main()
{
    std::cout << "======================================================" << std::endl;
    std::cout << "=== TEST_C27_GLTF_PIPELINE: E2E glTF RUNTIME TESTS ===" << std::endl;
    std::cout << "======================================================" << std::endl;

    CGltfModel model;
    TestPipelineInitialization(model);
    TestPlayMotionAndUpdate(model);
    TestDeformVertices(model);
    TestCheckEvents(model);
    TestGltfLoaderEventsAndMaterialsExtraction();
    TestBoneAttachmentAndSkeletonLink();

    std::cout << "======================================================" << std::endl;
    std::cout << "=== ALL PIPELINE TESTS PASSED SUCCESSFULLY (100%) ====" << std::endl;
    std::cout << "======================================================" << std::endl;

    return 0;
}
