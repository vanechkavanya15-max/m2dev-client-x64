#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>

#include "EterLib/Resource.h"
#include "EterGrnLib/Thing.h"
#include "EterGrnLib/LODController.h"
#include "EterModelLib/GltfModel.h"
#include "EterModelLib/GltfModelInstance.h"
#include "EterModelLib/GltfLoader.h"
#include "EterModelLib/SkeletalMath.h"

using namespace EterModelLib;

#include "EterLib/Camera.h"

float CCamera::CAMERA_MAX_DISTANCE = 2500.0f;

static bool ApproxEqual(float a, float b, float epsilon = 0.01f)
{
    return std::abs(a - b) < epsilon;
}

static bool ApproxEqualVec3(const float3& a, float x, float y, float z, float epsilon = 0.01f)
{
    return ApproxEqual(a.x, x, epsilon) &&
           ApproxEqual(a.y, y, epsilon) &&
           ApproxEqual(a.z, z, epsilon);
}

static bool FileExists(const std::string& path)
{
    std::ifstream f(path.c_str(), std::ios::binary);
    return f.good();
}

static float4x4 MakeIdentityFloat4x4()
{
    float4x4 mat;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            mat.m[r][c] = (r == c) ? 1.0f : 0.0f;
    return mat;
}

static float4x4 MakeTranslationFloat4x4(float tx, float ty, float tz)
{
    float4x4 mat = MakeIdentityFloat4x4();
    mat.m[3][0] = tx;
    mat.m[3][1] = ty;
    mat.m[3][2] = tz;
    return mat;
}

// ------------------------------------------------------------------------------------------------
// Budowa syntetycznego modelu wojownika z pelnym szkieletem:
// Kosci: Root (0), Bip01 Spine (1), Bip01 Head (2), Bip01 R Hand (3)
// ------------------------------------------------------------------------------------------------
static void BuildWarriorBodyData(GltfModelData& data)
{
    data.name = "WarriorBody";

    // 1 Podsiatka
    GltfSubmesh mesh;
    mesh.name = "WarriorBodyMesh";
    mesh.materialIndex = 0;
    mesh.vertexOffset = 0;
    mesh.vertexCount = 5;
    mesh.indexOffset = 0;
    mesh.indexCount = 6;
    data.submeshes.push_back(mesh);

    data.indices = { 0, 1, 2, 0, 2, 3 };

    // Szkielet
    // Kosc 0: Root (brak rodzica)
    GltfJoint jRoot;
    jRoot.name = "Root";
    jRoot.parentIndex = -1;
    jRoot.inverseBindMatrix = MakeIdentityFloat4x4();

    // Kosc 1: Bip01 Spine (dziecko Root, przesuniete w gore o Y=10)
    GltfJoint jSpine;
    jSpine.name = "Bip01 Spine";
    jSpine.parentIndex = 0;
    jSpine.inverseBindMatrix = MakeTranslationFloat4x4(0.0f, -10.0f, 0.0f); // inverse(T(0,10,0))

    // Kosc 2: Bip01 Head (dziecko Spine, przesuniete w gore o kolejne Y=10 -> world Y=20)
    GltfJoint jHead;
    jHead.name = "Bip01 Head";
    jHead.parentIndex = 1;
    jHead.inverseBindMatrix = MakeTranslationFloat4x4(0.0f, -20.0f, 0.0f); // inverse(T(0,20,0))

    // Kosc 3: Bip01 R Hand (dziecko Spine, przesuniete w prawo o X=15, world Y=10)
    GltfJoint jHand;
    jHand.name = "Bip01 R Hand";
    jHand.parentIndex = 1;
    jHand.inverseBindMatrix = MakeTranslationFloat4x4(-15.0f, -10.0f, 0.0f); // inverse(T(15,10,0))

    data.skin.name = "WarriorSkeleton";
    data.skin.joints.push_back(jRoot);
    data.skin.joints.push_back(jSpine);
    data.skin.joints.push_back(jHead);
    data.skin.joints.push_back(jHand);

    // Wierzcholki
    // V0: Przyczepiony 100% do Root (0,0,0)
    GltfVertex v0;
    v0.position = { 0.0f, 0.0f, 0.0f };
    v0.normal = { 0.0f, 1.0f, 0.0f };
    v0.jointIndices = { 0, 0, 0, 0 };
    v0.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

    // V1: Przyczepiony 100% do Spine (0,10,0)
    GltfVertex v1;
    v1.position = { 0.0f, 10.0f, 0.0f };
    v1.normal = { 0.0f, 1.0f, 0.0f };
    v1.jointIndices = { 1, 0, 0, 0 };
    v1.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

    // V2: Przyczepiony 100% do Head (0,20,0)
    GltfVertex v2;
    v2.position = { 0.0f, 20.0f, 0.0f };
    v2.normal = { 0.0f, 1.0f, 0.0f };
    v2.jointIndices = { 2, 0, 0, 0 };
    v2.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

    // V3: Przyczepiony 100% do R Hand (15,10,0)
    GltfVertex v3;
    v3.position = { 15.0f, 10.0f, 0.0f };
    v3.normal = { 1.0f, 0.0f, 0.0f };
    v3.jointIndices = { 3, 0, 0, 0 };
    v3.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

    // V4: Wagi mieszane 50% Spine, 50% R Hand (15,10,0)
    GltfVertex v4;
    v4.position = { 15.0f, 10.0f, 0.0f };
    v4.normal = { 1.0f, 0.0f, 0.0f };
    v4.jointIndices = { 3, 1, 0, 0 };
    v4.jointWeights = { 0.5f, 0.5f, 0.0f, 0.0f };

    data.vertices.push_back(v0);
    data.vertices.push_back(v1);
    data.vertices.push_back(v2);
    data.vertices.push_back(v3);
    data.vertices.push_back(v4);

    // Animacja "attack_combo"
    // Czas trwania 1.0s
    // Rotacja kosci dloni (Bip01 R Hand, index 3) wokol osi Z:
    //  t=0.0s: kat 0 st. -> Quat(0, 0, 0, 1)
    //  t=0.5s: kat 90 st. -> Quat(0, 0, 0.7071068, 0.7071068)
    //  t=1.0s: kat 180 st. -> Quat(0, 0, 1.0, 0.0)
    GltfMotionData anim;
    anim.name = "attack_combo";
    anim.duration = 1.0f;

    GltfAnimationChannel chRootT;
    chRootT.jointIndex = 0;
    chRootT.pathType = GltfAnimationPathType::Translation;
    chRootT.times = { 0.0f, 1.0f };
    chRootT.values = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
    anim.channels.push_back(chRootT);

    GltfAnimationChannel chSpineT;
    chSpineT.jointIndex = 1;
    chSpineT.pathType = GltfAnimationPathType::Translation;
    chSpineT.times = { 0.0f, 1.0f };
    chSpineT.values = { { 0, 10.0f, 0, 0 }, { 0, 10.0f, 0, 0 } };
    anim.channels.push_back(chSpineT);

    GltfAnimationChannel chHeadT;
    chHeadT.jointIndex = 2;
    chHeadT.pathType = GltfAnimationPathType::Translation;
    chHeadT.times = { 0.0f, 1.0f };
    chHeadT.values = { { 0, 10.0f, 0, 0 }, { 0, 10.0f, 0, 0 } };
    anim.channels.push_back(chHeadT);

    GltfAnimationChannel chHandT;
    chHandT.jointIndex = 3;
    chHandT.pathType = GltfAnimationPathType::Translation;
    chHandT.times = { 0.0f, 1.0f };
    chHandT.values = { { 15.0f, 0, 0, 0 }, { 15.0f, 0, 0, 0 } };
    anim.channels.push_back(chHandT);

    GltfAnimationChannel chHandR;
    chHandR.jointIndex = 3;
    chHandR.pathType = GltfAnimationPathType::Rotation;
    chHandR.times = { 0.0f, 0.5f, 1.0f };
    chHandR.values = {
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 0.7071068f, 0.7071068f },
        { 0.0f, 0.0f, 1.0f, 0.0f }
    };
    anim.channels.push_back(chHandR);

    anim.events.push_back({ 0.20f, "SOUND", "sound/pc/warrior/sword_swing.wav" });
    anim.events.push_back({ 0.45f, "ATTACK", "hit_slash" });

    data.motions.push_back(anim);
}

// ------------------------------------------------------------------------------------------------
// Budowa modelu miecza do podpiecia pod gniazdo dloni (equip_right / Bip01 R Hand)
// ------------------------------------------------------------------------------------------------
static void BuildWeaponSwordData(GltfModelData& data)
{
    data.name = "WarriorSword";

    GltfSubmesh mesh;
    mesh.name = "SwordMesh";
    mesh.materialIndex = 0;
    mesh.vertexOffset = 0;
    mesh.vertexCount = 3;
    mesh.indexOffset = 0;
    mesh.indexCount = 3;
    data.submeshes.push_back(mesh);

    data.indices = { 0, 1, 2 };

    // Wierzcholki ostrza (lokalnie wzgledem rekojesci w (0,0,0))
    GltfVertex v0;
    v0.position = { 0.0f, 0.0f, 0.0f }; // rekojesc / punkt mocowania
    v0.normal = { 0.0f, 1.0f, 0.0f };
    v0.jointWeights = { 0, 0, 0, 0 };

    GltfVertex v1;
    v1.position = { 20.0f, 0.0f, 0.0f }; // czubek ostrza (dlugosc 20 w osi X)
    v1.normal = { 0.0f, 1.0f, 0.0f };
    v1.jointWeights = { 0, 0, 0, 0 };

    GltfVertex v2;
    v2.position = { 0.0f, 2.0f, 0.0f }; // jelce
    v2.normal = { 0.0f, 1.0f, 0.0f };
    v2.jointWeights = { 0, 0, 0, 0 };

    data.vertices.push_back(v0);
    data.vertices.push_back(v1);
    data.vertices.push_back(v2);
}

// ------------------------------------------------------------------------------------------------
// Budowa modelu wlosow (Hair Link) - rigged do kosci Bip01 Head
// ------------------------------------------------------------------------------------------------
static void BuildWarriorHairData(GltfModelData& data)
{
    data.name = "WarriorHair";

    GltfSubmesh mesh;
    mesh.name = "HairMesh";
    mesh.materialIndex = 0;
    mesh.vertexOffset = 0;
    mesh.vertexCount = 3;
    mesh.indexOffset = 0;
    mesh.indexCount = 3;
    data.submeshes.push_back(mesh);

    data.indices = { 0, 1, 2 };

    // Szkielet wlosow ze spojnimi nazwami kosci
    GltfJoint jRoot;
    jRoot.name = "Root";
    jRoot.parentIndex = -1;
    jRoot.inverseBindMatrix = MakeIdentityFloat4x4();

    GltfJoint jSpine;
    jSpine.name = "Bip01 Spine";
    jSpine.parentIndex = 0;
    jSpine.inverseBindMatrix = MakeTranslationFloat4x4(0.0f, -10.0f, 0.0f);

    GltfJoint jHead;
    jHead.name = "Bip01 Head";
    jHead.parentIndex = 1;
    jHead.inverseBindMatrix = MakeTranslationFloat4x4(0.0f, -20.0f, 0.0f);

    data.skin.name = "HairSkeleton";
    data.skin.joints.push_back(jRoot);
    data.skin.joints.push_back(jSpine);
    data.skin.joints.push_back(jHead);

    // Wierzcholki wlosow przyczepione 100% do kosci Bip01 Head (indeks 2)
    GltfVertex v0;
    v0.position = { 0.0f, 22.0f, 0.0f }; // czubek glowy
    v0.normal = { 0.0f, 1.0f, 0.0f };
    v0.jointIndices = { 2, 0, 0, 0 };
    v0.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

    GltfVertex v1;
    v1.position = { 0.0f, 25.0f, 0.0f }; // koniec kosmyka
    v1.normal = { 0.0f, 1.0f, 0.0f };
    v1.jointIndices = { 2, 0, 0, 0 };
    v1.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

    GltfVertex v2;
    v2.position = { 2.0f, 22.0f, 0.0f };
    v2.normal = { 1.0f, 0.0f, 0.0f };
    v2.jointIndices = { 2, 0, 0, 0 };
    v2.jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

    data.vertices.push_back(v0);
    data.vertices.push_back(v1);
    data.vertices.push_back(v2);
}

// ------------------------------------------------------------------------------------------------
// TEST 1: Wczytanie modelu z szkieletem do CGraphicThing i powiazanie z CGrannyLODController
// ------------------------------------------------------------------------------------------------
void TestModelLoadingAndBinding(CGraphicThing*& outBodyThing, CGrannyLODController& outBodyController)
{
    std::cout << "[RUN] Test 1: Ladowanie modelu ze szkieletem do CGraphicThing i CGrannyLODController..." << std::endl;

    GltfModelData bodyData;
    BuildWarriorBodyData(bodyData);

    outBodyThing = new CGraphicThing("warrior_body.glb");
    outBodyThing->AddReferenceOnly();
    bool createOk = outBodyThing->CreateFromGltfModelData(bodyData);
    assert(createOk && "CGraphicThing::CreateFromGltfModelData musi sie powiesc");

    assert(outBodyThing->IsGltf() && "CGraphicThing musi byc rozpoznany jako glTF");
    assert(outBodyThing->GetModelCount() == 1 && "Liczba modeli w CGraphicThing musi wynosic 1");
    assert(outBodyThing->GetMotionCount() == 1 && "Liczba animacji w CGraphicThing musi wynosic 1");
    assert(outBodyThing->GetGltfModelPointer() != nullptr && "Wskaznik do CGltfModel nie moze byc pusty");

    // Powiazanie z CGrannyLODController
    outBodyController.AddModel(outBodyThing, 0);

    assert(outBodyController.IsGltf() && "CGrannyLODController musi dzialac w trybie glTF");
    assert(outBodyController.isModelInstance() && "isModelInstance() musi zwracac TRUE dla glTF");
    assert(outBodyController.GetGltfModelInstance() != nullptr && "GetGltfModelInstance() nie moze byc NULL");
    assert(outBodyController.GetVertexCount() == 5 && "Liczba wierzcholkow w kontrolerze musi wynosic 5");

    std::cout << "[PASS] Test 1: Poprawnie wczytano model i powiazano z CGrannyLODController." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// TEST 2: Przeliczanie klatek animacji i deformacje wierzcholkow w UpdateTime()
// ------------------------------------------------------------------------------------------------
void TestAnimationAndDeformation(CGraphicThing* pBodyThing, CGrannyLODController& bodyController)
{
    std::cout << "[RUN] Test 2: Przeliczanie klatek animacji i deformacja wierzcholkow w UpdateTime()..." << std::endl;

    const CGrannyMotion* pMotion = pBodyThing->GetMotionPointer(0);
    assert(pMotion != nullptr && "Pobranie CGrannyMotion dla indeksu 0 musi sie powiesc");
    assert(std::string(pMotion->GetName()) == "attack_combo");
    assert(ApproxEqual(pMotion->GetDuration(), 1.0f));

    bodyController.SetMotionPointer(pMotion, 0.0f, 1, 1.0f);

    // Stan poczatkowy t = 0.0s
    bodyController.UpdateTime(0.0f);
    const auto& initVerts = bodyController.GetDeformedVertices();
    assert(initVerts.size() == 5);

    // Wierzcholek dloni V3 poczatkowo w (15, 10, 0)
    assert(ApproxEqualVec3(initVerts[3].position, 15.0f, 10.0f, 0.0f));
    assert(ApproxEqualVec3(initVerts[3].normal, 1.0f, 0.0f, 0.0f));

    // Krok w czasie: dt = 0.5s -> rotacja dloni o 90 stopni wokol Z
    bodyController.UpdateTime(0.5f);
    assert(ApproxEqual(bodyController.GetGltfModelInstance()->GetCurrentTime(), 0.5f));

    // Weryfikacja macierzy kosci dloni "Bip01 R Hand" (indeks 3) po rotacji o 90 stopni
    const float* pHandMat = bodyController.GetBoneMatrixPointer("Bip01 R Hand");
    assert(pHandMat != nullptr && "GetBoneMatrixPointer('Bip01 R Hand') nie moze byc NULL");

    // Macierz rotacji o 90 st. wokol Z: m[0][0]=0, m[0][1]=-1, m[1][0]=1, m[1][1]=0
    // Pozycja kosci dloni: Spine(0,10,0) + Hand(15,0,0) = (15, 10, 0)
    assert(ApproxEqual(pHandMat[0], 0.0f));
    assert(ApproxEqual(pHandMat[1], -1.0f));
    assert(ApproxEqual(pHandMat[4], 1.0f));
    assert(ApproxEqual(pHandMat[5], 0.0f));
    assert(ApproxEqual(pHandMat[12], 15.0f));
    assert(ApproxEqual(pHandMat[13], 10.0f));

    // Weryfikacja deformacji wierzcholkow
    const auto& defVerts90 = bodyController.GetDeformedVertices();
    assert(defVerts90.size() == 5);

    // V3 (przyczepiony 100% do kosci R Hand) obrocony o 90 stopni wzgledem pozycji kosci (15,10,0)
    // Brak offsetu wzgledem bind pose -> pozycja wierzcholka to dokladnie pozycja kosci: (15, 10, 0)
    assert(ApproxEqualVec3(defVerts90[3].position, 15.0f, 10.0f, 0.0f));
    // Normalna (1,0,0) obrocona o 90 st. wokol Z -> (0, -1, 0)
    assert(ApproxEqualVec3(defVerts90[3].normal, 0.0f, -1.0f, 0.0f));

    // Sprawdzenie braku NaN / Inf i normalizacji
    for (size_t i = 0; i < defVerts90.size(); ++i)
    {
        const auto& v = defVerts90[i];
        assert(!std::isnan(v.position.x) && !std::isnan(v.position.y) && !std::isnan(v.position.z));
        float len = std::sqrt(v.normal.x * v.normal.x + v.normal.y * v.normal.y + v.normal.z * v.normal.z);
        assert(ApproxEqual(len, 1.0f, 0.05f) && "Wektor normalny musi byc znormalizowany");
    }

    // Kolejny krok: dt = 0.5s -> t = 1.0s (rotacja o 180 stopni wokol Z)
    bodyController.UpdateTime(0.5f);
    assert(ApproxEqual(bodyController.GetGltfModelInstance()->GetCurrentTime(), 1.0f));

    const auto& defVerts180 = bodyController.GetDeformedVertices();
    // Normalna obrocona o 180 st. wokol Z -> (-1, 0, 0)
    assert(ApproxEqualVec3(defVerts180[3].normal, -1.0f, 0.0f, 0.0f));

    std::cout << "[PASS] Test 2: Poprawnie przeliczono klatki animacji i deformacje wierzcholkow." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// TEST 3: Pobieranie macierzy kosci GetBoneMatrixPointer dla podpinania broni (gniazdo dloni)
// ------------------------------------------------------------------------------------------------
void TestWeaponBoneAttachment(CGraphicThing*& outWeaponThing, CGrannyLODController& bodyController, CGrannyLODController& weaponController)
{
    std::cout << "[RUN] Test 3: GetBoneMatrixPointer dla podpinania broni do gniazda dloni..." << std::endl;

    // 1. Sprawdzenie pobierania macierzy kosci za pomoca nazwy oraz indeksu
    int handBoneIndex = -1;
    bool foundIndex = bodyController.GetBoneIndexByName("Bip01 R Hand", &handBoneIndex);
    assert(foundIndex && handBoneIndex == 3);

    const float* pMatByIndex = bodyController.GetBoneMatrixPointer(handBoneIndex);
    const float* pMatByName = bodyController.GetBoneMatrixPointer("Bip01 R Hand");
    assert(pMatByIndex != nullptr);
    assert(pMatByName != nullptr);
    assert(pMatByIndex == pMatByName && "Wskaznik pobrany po nazwie i indeksie musi byc identyczny");

    // 2. Utworzenie modelu miecza
    GltfModelData weaponData;
    BuildWeaponSwordData(weaponData);

    outWeaponThing = new CGraphicThing("warrior_sword.glb");
    outWeaponThing->AddReferenceOnly();
    bool weaponCreateOk = outWeaponThing->CreateFromGltfModelData(weaponData);
    assert(weaponCreateOk);

    weaponController.AddModel(outWeaponThing, 0);
    assert(weaponController.IsGltf());

    // 3. Podpiecie broni pod gniazdo dloni w kontrolerze glownym
    bodyController.AttachModelInstance(&weaponController, "Bip01 R Hand");

    assert(weaponController.GetAttachedParentModel() == &bodyController);

    // 4. Przeliczenie klatki broni - deformacja wierzcholkow miecza wedlug gniazda dloni
    weaponController.UpdateTime(0.0f);

    const auto& weaponVerts = weaponController.GetDeformedVertices();
    assert(weaponVerts.size() == 3);

    // Wierzcholek 0 (rekojesc w (0,0,0) lokalnie) po deformacji musi znalezc sie DOKLADNIE
    // w pozycji kosci dloni: (pMatByName[12], pMatByName[13], pMatByName[14])
    assert(ApproxEqual(weaponVerts[0].position.x, pMatByName[12]));
    assert(ApproxEqual(weaponVerts[0].position.y, pMatByName[13]));
    assert(ApproxEqual(weaponVerts[0].position.z, pMatByName[14]));

    std::cout << "  - Pozycja gniazda dloni: (" << pMatByName[12] << ", " << pMatByName[13] << ", " << pMatByName[14] << ")" << std::endl;
    std::cout << "  - Pozycja rekojesci broni: (" << weaponVerts[0].position.x << ", " << weaponVerts[0].position.y << ", " << weaponVerts[0].position.z << ")" << std::endl;

    // Odpiecie modelu broni i weryfikacja
    bodyController.DetachModelInstance(&weaponController);
    assert(weaponController.GetAttachedParentModel() == nullptr);

    std::cout << "[PASS] Test 3: GetBoneMatrixPointer i podpinanie broni do dloni zakonczone pomyslnie." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// TEST 4: Synchronizacja wlosow (Hair Link)
// ------------------------------------------------------------------------------------------------
void TestHairLinkSynchronization(CGraphicThing*& outHairThing, CGrannyLODController& bodyController, CGrannyLODController& hairController)
{
    std::cout << "[RUN] Test 4: Synchronizacja wlosow (Hair Link)..." << std::endl;

    // 1. Przygotowanie modelu wlosow
    GltfModelData hairData;
    BuildWarriorHairData(hairData);

    outHairThing = new CGraphicThing("warrior_hair.glb");
    outHairThing->AddReferenceOnly();
    bool hairCreateOk = outHairThing->CreateFromGltfModelData(hairData);
    assert(hairCreateOk);

    // 2. Powiazanie wlosow z kontrolerem ciala za pomoca Hair Link (pSkelLODController = &bodyController)
    hairController.AddModel(outHairThing, 0, &bodyController);
    assert(hairController.IsGltf());
    assert(hairController.GetGltfModelInstance() != nullptr);
    assert(hairController.GetGltfModelInstance()->GetLinkedModelInstance() == bodyController.GetGltfModelInstance());

    // 3. Sprawdzenie stanu poczatkowego wlosow
    bodyController.SetLocalTime(0.0f);
    bodyController.UpdateTime(0.0f);
    hairController.UpdateTime(0.0f);

    const auto& hairVertsInit = hairController.GetDeformedVertices();
    assert(hairVertsInit.size() == 3);
    // Czubek glowy w bind pose: (0, 22, 0)
    assert(ApproxEqualVec3(hairVertsInit[0].position, 0.0f, 22.0f, 0.0f));

    // 4. Pobranie macierzy kosci glowy z ciala i porownanie z macierza kosci glowy w kontrolerze wlosow
    const float* pBodyHeadMat = bodyController.GetBoneMatrixPointer("Bip01 Head");
    const float* pHairHeadMat = hairController.GetBoneMatrixPointer("Bip01 Head");
    assert(pBodyHeadMat != nullptr && pHairHeadMat != nullptr);

    for (int idx = 0; idx < 16; ++idx)
    {
        assert(ApproxEqual(pBodyHeadMat[idx], pHairHeadMat[idx]));
    }

    // 5. Ruch w czasie ciala
    bodyController.UpdateTime(0.3f);
    hairController.UpdateTime(0.0f); // synchronizacja z zaktualizowanym cialem

    const float* pBodyHeadMatT = bodyController.GetBoneMatrixPointer("Bip01 Head");
    const float* pHairHeadMatT = hairController.GetBoneMatrixPointer("Bip01 Head");

    for (int idx = 0; idx < 16; ++idx)
    {
        assert(ApproxEqual(pBodyHeadMatT[idx], pHairHeadMatT[idx]));
    }

    const auto& hairVertsT = hairController.GetDeformedVertices();
    assert(hairVertsT.size() == 3);
    assert(!std::isnan(hairVertsT[0].position.x) && !std::isnan(hairVertsT[0].position.y) && !std::isnan(hairVertsT[0].position.z));

    std::cout << "  - Macierz glowy ciala [Y translacja]: " << pBodyHeadMatT[13] << std::endl;
    std::cout << "  - Macierz glowy wlosow [Y translacja]: " << pHairHeadMatT[13] << std::endl;
    std::cout << "  - Pozycja czubka wlosow po synchronizacji: (" << hairVertsT[0].position.x << ", " << hairVertsT[0].position.y << ", " << hairVertsT[0].position.z << ")" << std::endl;

    std::cout << "[PASS] Test 4: Synchronizacja wlosow (Hair Link) dziala z pelna zgodnoscia szkieletowa." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// TEST 5: Weryfikacja rzeczywistego modelu Bossa i animacji ataku z zrzutu gry
// ------------------------------------------------------------------------------------------------
void TestRealBossAssetBridge()
{
    std::cout << "[RUN] Test 5: Weryfikacja rzeczywistego modelu Bossa (.glb) i mostka D3D..." << std::endl;

    const std::string bossGlbPath = "E:/Alune-AkademiaDUMP/monster/asek_alune_update_boss/asek_alune_update_boss.glb";
    const std::string attackGlbPath = "E:/Alune-AkademiaDUMP/monster/asek_alune_update_boss/attack.glb";

    if (!FileExists(bossGlbPath))
    {
        std::cout << "  [INFO] Brak pliku bossa na dysku (" << bossGlbPath << ") - pomijanie testu zrzutu." << std::endl;
        return;
    }

    CGraphicThing* pBossThing = new CGraphicThing(bossGlbPath.c_str());
    pBossThing->AddReferenceOnly();
    bool loadOk = pBossThing->LoadFromFile(bossGlbPath.c_str());
    assert(loadOk && "Ladowanie asek_alune_update_boss.glb do CGraphicThing musi sie powiesc");

    assert(pBossThing->IsGltf());
    assert(pBossThing->GetModelCount() > 0);

    CGrannyLODController bossController;
    bossController.AddModel(pBossThing, 0);

    assert(bossController.IsGltf());
    assert(bossController.GetVertexCount() > 0);
    std::cout << "  - Zaladowano model Bossa: " << bossController.GetVertexCount() << " wierzcholkow" << std::endl;

    // Test pobierania macierzy kosci szkieletu bossa
    const float* pRootMat = bossController.GetBoneMatrixPointer(0);
    assert(pRootMat != nullptr && "Macierz kosci 0 bossa nie moze byc NULL");

    // Jesli dostepna jest animacja ataku
    if (FileExists(attackGlbPath))
    {
        GltfLoader loader;
        GltfModelData attackData;
        if (loader.LoadFromFile(attackGlbPath, attackData) && !attackData.motions.empty())
        {
            pBossThing->RegisterMotion(attackData.motions[0]);
            assert(pBossThing->GetMotionCount() >= 1);

            const CGrannyMotion* pBossAttackMotion = pBossThing->GetMotionPointer(0);
            assert(pBossAttackMotion != nullptr);

            bossController.SetMotionPointer(pBossAttackMotion, 0.0f, 0, 1.0f);

            // Wykonanie 5 klatek symulacji UpdateTime()
            const float dt = 1.0f / 30.0f;
            for (int f = 0; f < 5; ++f)
            {
                bossController.UpdateTime(dt);
                const auto& defVerts = bossController.GetDeformedVertices();
                assert(!defVerts.empty());
                assert(!std::isnan(defVerts[0].position.x));
            }
            std::cout << "  - Wykonano 5 klatek deformacji animacji ataku Bossa bez bledow" << std::endl;
        }
    }

    pBossThing->Release();
    std::cout << "[PASS] Test 5: Rzeczywisty model Bossa zintegrowany pomyslnie z CGraphicThing i CGrannyLODController." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// TEST 6: Odwrocona kolejnosc kosci w podpietym modelu (Non-topological joint ordering) i Double-Sided
// ------------------------------------------------------------------------------------------------
void TestReverseJointOrderAttachmentAndDoubleSided(CGrannyLODController& bodyController)
{
    std::cout << "[RUN] Test 6: Wielokosciowy attachment z odwrocona kolejnoscia kosci i material doubleSided..." << std::endl;

    // Przygotowanie modelu luku z 2 kosciami:
    // Kosc 0: Bow_Tip (dziecko kosci Bow_Handle o indeksie 1!) -> parentIndex = 1
    // Kosc 1: Bow_Handle (root attachmentu, parentIndex = -1)
    GltfModelData bowData;
    bowData.name = "d:/ymir work/weapon/warrior_bow.glb";

    GltfSubmesh bowMesh;
    bowMesh.name = "BowMesh";
    bowMesh.materialIndex = 0;
    bowMesh.vertexOffset = 0;
    bowMesh.vertexCount = 2;
    bowMesh.indexOffset = 0;
    bowMesh.indexCount = 2;
    bowData.submeshes.push_back(bowMesh);

    GltfMaterial bowMat;
    bowMat.name = "BowMaterial";
    bowMat.doubleSided = true;
    bowData.materials.push_back(bowMat);

    // Kosc 0: Bow_Tip (dziecko)
    GltfJoint jTip;
    jTip.name = "Bow_Tip";
    jTip.parentIndex = 1; // rodzic to kosc 1 (indeks wyzszy niz dziecko!)
    jTip.localTranslation = { 0.0f, 5.0f, 0.0f };
    jTip.inverseBindMatrix = MakeTranslationFloat4x4(0.0f, -5.0f, 0.0f);
    bowData.skin.joints.push_back(jTip);

    // Kosc 1: Bow_Handle (rodzic)
    GltfJoint jHandle;
    jHandle.name = "Bow_Handle";
    jHandle.parentIndex = -1;
    jHandle.localTranslation = { 0.0f, 0.0f, 0.0f };
    jHandle.inverseBindMatrix = MakeIdentityFloat4x4();
    bowData.skin.joints.push_back(jHandle);

    CGraphicThing* pBowThing = new CGraphicThing(bowData.name.c_str());
    pBowThing->AddReferenceOnly();
    bool created = pBowThing->CreateFromGltfModelData(bowData);
    assert(created);

    // Sprawdzenie flagi doubleSided i nazwy modelu
    assert(pBowThing->GetGltfModelPointer()->GetModelData().materials[0].doubleSided == true);
    assert(pBowThing->GetGltfModelPointer()->GetName() == "d:/ymir work/weapon/warrior_bow.glb");

    CGrannyLODController bowController;
    bowController.AddModel(pBowThing, 0);

    // Podpiecie luku do reki wojownika
    bodyController.AttachModelInstance(&bowController, "Bip01 R Hand");
    bowController.UpdateTime(0.0f);

    const Matrix4x4* pHandMat = bodyController.GetGltfModelInstance()->GetBoneMatrixPointer("Bip01 R Hand");
    assert(pHandMat != nullptr);

    // Kosc 1 (Handle) powinna miec dokladnie macierz dloni
    const Matrix4x4* pHandleWorld = bowController.GetGltfModelInstance()->GetBoneMatrixPointer(1);
    assert(pHandleWorld != nullptr);
    assert(ApproxEqual(pHandleWorld->m[3][0], pHandMat->m[3][0]));
    assert(ApproxEqual(pHandleWorld->m[3][1], pHandMat->m[3][1]));
    assert(ApproxEqual(pHandleWorld->m[3][2], pHandMat->m[3][2]));

    // Kosc 0 (Tip) powinna miec macierz dloni przesunieta o T(0, 5, 0)
    const Matrix4x4* pTipWorld = bowController.GetGltfModelInstance()->GetBoneMatrixPointer(0);
    assert(pTipWorld != nullptr);
    assert(ApproxEqual(pTipWorld->m[3][1], pHandMat->m[3][1] + 5.0f));

    bodyController.DetachModelInstance(&bowController);
    pBowThing->Release();

    std::cout << "[PASS] Test 6: Poprawnie zpropagowano transformacje dla odwroconej hierarchii kosci oraz potwierdzono doubleSided." << std::endl;
}

// ------------------------------------------------------------------------------------------------
// Glowny punkt wejscia testu integracyjnego
// ------------------------------------------------------------------------------------------------
int main()
{
    std::cout << "==========================================================================" << std::endl;
    std::cout << "=== TEST_C29_GLTF_D3D_BRIDGE: INTEGRACJA glTF / CGrannyLODController   ===" << std::endl;
    std::cout << "==========================================================================" << std::endl;

    // Wylaczenie opoznionego usuwania przez CResourceManager w srodowisku testowym
    CResource::SetDeleteImmediately(true);

    CGraphicThing* pBodyThing = nullptr;
    CGraphicThing* pWeaponThing = nullptr;
    CGraphicThing* pHairThing = nullptr;

    CGrannyLODController bodyController;
    CGrannyLODController weaponController;
    CGrannyLODController hairController;

    // Krok 1: Wczytanie modelu i powiazanie z kontrolerem
    TestModelLoadingAndBinding(pBodyThing, bodyController);

    // Krok 2: Animacja i deformacja wierzcholkow w UpdateTime
    TestAnimationAndDeformation(pBodyThing, bodyController);

    // Krok 3: GetBoneMatrixPointer dla podpinania broni (gniazdo dloni)
    TestWeaponBoneAttachment(pWeaponThing, bodyController, weaponController);

    // Krok 4: Synchronizacja wlosow (Hair Link)
    TestHairLinkSynchronization(pHairThing, bodyController, hairController);

    // Krok 5: Weryfikacja rzeczywistego bossa z gry
    TestRealBossAssetBridge();

    // Krok 6: Odwrocona hierarchia kosci attachmentu i material doubleSided
    TestReverseJointOrderAttachmentAndDoubleSided(bodyController);

    // Czyszczenie zasobow
    if (pBodyThing)
        pBodyThing->Release();
    if (pWeaponThing)
        pWeaponThing->Release();
    if (pHairThing)
        pHairThing->Release();

    std::cout << "==========================================================================" << std::endl;
    std::cout << "=== WSZYSTKIE TESTY MOSTKA glTF/D3D ZAKONCZONE SUKCESEM (100% PASS)    ===" << std::endl;
    std::cout << "==========================================================================" << std::endl;

    return 0;
}
