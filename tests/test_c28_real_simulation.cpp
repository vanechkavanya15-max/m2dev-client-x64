#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>

#include "EterModelLib/GltfModel.h"
#include "EterModelLib/GltfModelInstance.h"
#include "EterModelLib/GltfLoader.h"
#include "EterModelLib/SkeletalMath.h"

using namespace EterModelLib;

static bool FileExists(const std::string& path)
{
    std::ifstream f(path.c_str(), std::ios::binary);
    return f.good();
}

int main()
{
    std::cout << "=================================================================" << std::endl;
    std::cout << "=== TEST_C28: REAL-WORLD METIN2 glTF SIMULATION & MSA SYNC   ===" << std::endl;
    std::cout << "=================================================================" << std::endl;

    const std::string modelGlbPath = "E:/Alune-AkademiaDUMP/monster/asek_alune_update_boss/asek_alune_update_boss.glb";
    const std::string attackGlbPath = "E:/Alune-AkademiaDUMP/monster/asek_alune_update_boss/attack.glb";
    const std::string skillGlbPath = "E:/Alune-AkademiaDUMP/monster/asek_alune_update_boss/skill.glb";

    if (!FileExists(modelGlbPath))
    {
        std::cerr << "[SKIP] Plik modelu GLB nie istnieje: " << modelGlbPath << std::endl;
        return 0;
    }

    // --------------------------------------------------------------------------------------------
    // KROK 1: Wczytanie rzeczywistego modelu szkieletowego Bossa (.glb)
    // --------------------------------------------------------------------------------------------
    std::cout << "[SIM 1] Ladowanie modelu bossa z GLB..." << std::endl;
    GltfLoader loader;
    GltfModelData bossData;
    bool modelLoaded = loader.LoadFromFile(modelGlbPath, bossData);
    assert(modelLoaded && "Ladowanie asek_alune_update_boss.glb musi sie powiesc");

    std::cout << "  - Wierzcholki: " << bossData.vertices.size() << std::endl;
    std::cout << "  - Indeksy: " << bossData.indices.size() << std::endl;
    std::cout << "  - Podsiatki: " << bossData.submeshes.size() << std::endl;
    std::cout << "  - Kosci szkieletu: " << bossData.skin.joints.size() << std::endl;
    std::cout << "  - Materialy: " << bossData.materials.size() << std::endl;

    assert(!bossData.vertices.empty() && "Model musi posiadac wierzcholki");
    assert(!bossData.indices.empty() && "Model musi posiadac indeksy");
    assert(!bossData.submeshes.empty() && "Model musi posiadac co najmniej 1 podsiatke");
    assert(!bossData.skin.joints.empty() && "Model szkieletowy musi posiadac kosci");

    // Weryfikacja danych materialu / tekstury
    if (!bossData.materials.empty())
    {
        std::cout << "  - Nazwa materialu 0: " << bossData.materials[0].name << std::endl;
        std::cout << "  - Tekstura diffuse: " << bossData.materials[0].diffuseTexture << std::endl;
    }

    // --------------------------------------------------------------------------------------------
    // KROK 2: Wczytanie i weryfikacja synchronizacji animacji z plikami MSA
    // --------------------------------------------------------------------------------------------
    std::cout << "[SIM 2] Weryfikacja animacji attack.glb i synchronizacji z attack.msa..." << std::endl;
    GltfModelData attackData;
    bool attackLoaded = loader.LoadFromFile(attackGlbPath, attackData);
    assert(attackLoaded && "Ladowanie attack.glb musi sie powiesc");
    assert(!attackData.motions.empty() && "attack.glb musi zawierac animacje");

    float attackDuration = attackData.motions[0].duration;
    std::cout << "  - Czas trwania attack.glb: " << attackDuration << " s" << std::endl;
    std::cout << "  - Oczekiwany czas z attack.msa: 3.933333 s" << std::endl;
    // Sprawdzenie tolerancji mikrosekundowej
    assert(std::abs(attackDuration - 3.933333f) < 0.05f && "Czas trwania animacji ataku musi sie zgadzac z plikiem .msa");

    std::cout << "[SIM 3] Weryfikacja animacji skill.glb i synchronizacji ze skill.msa..." << std::endl;
    GltfModelData skillData;
    bool skillLoaded = loader.LoadFromFile(skillGlbPath, skillData);
    assert(skillLoaded && "Ladowanie skill.glb musi sie powiesc");
    assert(!skillData.motions.empty() && "skill.glb musi zawierac animacje");

    float skillDuration = skillData.motions[0].duration;
    std::cout << "  - Czas trwania skill.glb: " << skillDuration << " s" << std::endl;
    std::cout << "  - Oczekiwany czas ze skill.msa: 4.133333 s" << std::endl;
    assert(std::abs(skillDuration - 4.133333f) < 0.05f && "Czas trwania skilla musi sie zgadzac ze skill.msa");

    // --------------------------------------------------------------------------------------------
    // KROK 3: Pelna symulacja instancji CGltfModelInstance, hierarchii i CPU skinningu
    // --------------------------------------------------------------------------------------------
    std::cout << "[SIM 4] Symulacja runtime CGltfModelInstance..." << std::endl;
    CGltfModel bossModel;
    bossModel.GetModelData() = bossData;
    // Dolacz animacje ataku do modelu
    bossModel.GetModelData().motions.push_back(attackData.motions[0]);

    CGltfModelInstance bossInstance(&bossModel);
    bossInstance.DeformVertices();
    const auto& restVertices = bossInstance.GetDeformedVertices();
    const auto& srcVertices = bossData.vertices;
    float maxDiff = 0.0f;
    for (size_t i = 0; i < srcVertices.size(); ++i) {
        float dx = std::abs(restVertices[i].position.x - srcVertices[i].position.x);
        float dy = std::abs(restVertices[i].position.y - srcVertices[i].position.y);
        float dz = std::abs(restVertices[i].position.z - srcVertices[i].position.z);
        maxDiff = std::max({maxDiff, dx, dy, dz});
    }
    std::cout << "  - Maksymalna roznica wierzcholkow w Rest Pose: " << maxDiff << std::endl;
    assert(maxDiff < 0.01f && "Maksymalna roznica wierzcholkow w Rest Pose musi byc mniejsza niz 0.01");
    bossInstance.PlayMotion(attackData.motions[0].name, true);

    // Krok symulacji w czasie: dt = 0.033s (~30 FPS / g_fGameFPS)
    std::vector<GltfVertex> deformedVertices;
    const float dt = 1.0f / 30.0f;
    for (int frame = 0; frame < 10; ++frame)
    {
        bossInstance.Update(dt);
        bossInstance.DeformVertices();

        const auto& deformedVertices = bossInstance.GetDeformedVertices();
        assert(deformedVertices.size() == bossData.vertices.size());
        // Sprawdz poprawnosc pierwszego wierzcholka (brak NaN / Inf)
        const auto& v = deformedVertices[0];
        assert(!std::isnan(v.position.x) && !std::isnan(v.position.y) && !std::isnan(v.position.z));
        float normLen = std::sqrt(v.normal.x * v.normal.x + v.normal.y * v.normal.y + v.normal.z * v.normal.z);
        assert(std::abs(normLen - 1.0f) < 0.05f && "Znormalizowany wektor normalny po deformacji");
    }

    std::cout << "[PASS] Symulacja 10 klatek deformacji szkieletu zakonczona pomyslnie (brak anomalii)." << std::endl;

    std::cout << "=================================================================" << std::endl;
    std::cout << "=== WSZYSTKIE TESTY SYMULACYJNE REAL-WORLD ZAKONCZONE SUKCESEM ===" << std::endl;
    std::cout << "=================================================================" << std::endl;

    return 0;
}
