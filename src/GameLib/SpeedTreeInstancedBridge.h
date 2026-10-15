#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <cmath>

#include "Client/Graphics/HardwareMeshInstancer.h"

namespace GameLib
{

/**
 * @brief Os pionowa stosowana do wyliczania macierzy rotacji instancji drzewa.
 */
enum class TreeRotationAxis : uint8_t
{
    Z_Up = 0, // Standard silnika Metin2 (os Z pionowa)
    Y_Up = 1  // Standard Direct3D (os Y pionowa)
};

/**
 * @brief Wpis pojedynczego drzewa lub elementu roslinnosci zarejestrowanego do partii.
 */
struct TreeInstanceEntry
{
    uint32_t treeTypeId{0};
    float posX{0.0f};
    float posY{0.0f};
    float posZ{0.0f};
    float scale{1.0f};
    float rotationYaw{0.0f};
    uint32_t tintColor{0xFFFFFFFF};
    float windPhase{0.0f};
    uint32_t lodIndex{0};
};

/**
 * @brief Mapowanie typu drzewa na identyfikator siatki i materialu dla bucketa instancera.
 */
struct TreeMeshMapping
{
    uint32_t meshId{0};
    uint32_t materialId{0};
};

/**
 * @class SpeedTreeInstancedBridge
 * @brief Most laczacy podsystem SpeedTree z modulem sprzetowego instancingu siatek (HardwareMeshInstancer).
 *
 * Zbiera dane roslinnosci z terenu swiata gry, przelicza pozycje, katy yaw i skale na
 * macierze transformacji swiata 4x4 (wierszowe, row-major dla Direct3D 9),
 * a nastepnie grupuje i przekazuje instancje do odpowiednich bucketow sprzetowych.
 */
class SpeedTreeInstancedBridge
{
public:
    SpeedTreeInstancedBridge() = default;
    ~SpeedTreeInstancedBridge() = default;

    SpeedTreeInstancedBridge(const SpeedTreeInstancedBridge&) = delete;
    SpeedTreeInstancedBridge& operator=(const SpeedTreeInstancedBridge&) = delete;

    SpeedTreeInstancedBridge(SpeedTreeInstancedBridge&&) noexcept = default;
    SpeedTreeInstancedBridge& operator=(SpeedTreeInstancedBridge&&) noexcept = default;

    /**
     * @brief Rejestruje pojedyncze drzewo do wyrysowania w biezacej ramce.
     */
    void RegisterTree(
        uint32_t treeTypeId,
        float posX,
        float posY,
        float posZ,
        float scale,
        float rotationYaw,
        uint32_t tintColor,
        float windPhase,
        uint32_t lodIndex);

    /**
     * @brief Konfiguruje przypisanie typu drzewa do identyfikatorow siatki i materialu.
     */
    void SetTreeTypeMapping(uint32_t treeTypeId, uint32_t meshId, uint32_t materialId);

    /**
     * @brief Ustawia os obrotu (domyslnie Z_Up dla silnika gry).
     */
    void SetRotationAxis(TreeRotationAxis axis) noexcept { m_rotationAxis = axis; }
    [[nodiscard]] TreeRotationAxis GetRotationAxis() const noexcept { return m_rotationAxis; }

    /**
     * @brief Przekazuje wszystkie zgromadzone instancje drzew do HardwareMeshInstancer.
     * @param instancer Modul instancingu docelowego.
     * @return Liczba wygenerowanych i przekazanych instancji.
     */
    size_t FlushTrees(Client::Graphics::HardwareMeshInstancer& instancer);

    /**
     * @brief Czysci wewnetrzna kolejke zarejestrowanych drzew.
     */
    void Clear() noexcept;

    // Gettery stanu
    [[nodiscard]] size_t GetRegisteredTreeCount() const noexcept { return m_trees.size(); }
    [[nodiscard]] const std::vector<TreeInstanceEntry>& GetRegisteredTrees() const noexcept { return m_trees; }

    /**
     * @brief Statyczna funkcja pomocnicza budujaca InstanceData z wyliczona macierza 4x4.
     */
    static Client::Graphics::InstanceData BuildInstanceData(
        const TreeInstanceEntry& entry,
        TreeRotationAxis axis = TreeRotationAxis::Z_Up) noexcept;

private:
    std::vector<TreeInstanceEntry> m_trees;
    std::unordered_map<uint32_t, TreeMeshMapping> m_typeMappings;
    TreeRotationAxis m_rotationAxis{TreeRotationAxis::Z_Up};
};

} // namespace GameLib
