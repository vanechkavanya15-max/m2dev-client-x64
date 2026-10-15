#include "SpeedTreeInstancedBridge.h"

#include <cmath>

namespace GameLib
{

void SpeedTreeInstancedBridge::RegisterTree(
    uint32_t treeTypeId,
    float posX,
    float posY,
    float posZ,
    float scale,
    float rotationYaw,
    uint32_t tintColor,
    float windPhase,
    uint32_t lodIndex)
{
    m_trees.push_back(TreeInstanceEntry{
        .treeTypeId = treeTypeId,
        .posX = posX,
        .posY = posY,
        .posZ = posZ,
        .scale = scale,
        .rotationYaw = rotationYaw,
        .tintColor = tintColor,
        .windPhase = windPhase,
        .lodIndex = lodIndex
    });
}

void SpeedTreeInstancedBridge::SetTreeTypeMapping(uint32_t treeTypeId, uint32_t meshId, uint32_t materialId)
{
    m_typeMappings[treeTypeId] = TreeMeshMapping{meshId, materialId};
}

size_t SpeedTreeInstancedBridge::FlushTrees(Client::Graphics::HardwareMeshInstancer& instancer)
{
    if (m_trees.empty())
    {
        return 0;
    }

    const size_t count = m_trees.size();

    for (const auto& tree : m_trees)
    {
        uint32_t meshId = tree.treeTypeId;
        uint32_t materialId = 0;

        const auto it = m_typeMappings.find(tree.treeTypeId);
        if (it != m_typeMappings.end())
        {
            meshId = it->second.meshId;
            materialId = it->second.materialId;
        }

        const auto instanceData = BuildInstanceData(tree, m_rotationAxis);
        instancer.AddInstance(meshId, materialId, tree.lodIndex, instanceData);
    }

    m_trees.clear();
    return count;
}

void SpeedTreeInstancedBridge::Clear() noexcept
{
    m_trees.clear();
}

Client::Graphics::InstanceData SpeedTreeInstancedBridge::BuildInstanceData(
    const TreeInstanceEntry& entry,
    TreeRotationAxis axis) noexcept
{
    Client::Graphics::InstanceData data{};

    const float cosYaw = std::cos(entry.rotationYaw);
    const float sinYaw = std::sin(entry.rotationYaw);
    const float s = entry.scale;

    if (axis == TreeRotationAxis::Z_Up)
    {
        // Konwencja silnika gry Metin2 (os Z skierowana w gore)
        data.worldRow0[0] = s * cosYaw;
        data.worldRow0[1] = s * sinYaw;
        data.worldRow0[2] = 0.0f;
        data.worldRow0[3] = 0.0f;

        data.worldRow1[0] = -s * sinYaw;
        data.worldRow1[1] = s * cosYaw;
        data.worldRow1[2] = 0.0f;
        data.worldRow1[3] = 0.0f;

        data.worldRow2[0] = 0.0f;
        data.worldRow2[1] = 0.0f;
        data.worldRow2[2] = s;
        data.worldRow2[3] = 0.0f;

        data.worldRow3[0] = entry.posX;
        data.worldRow3[1] = entry.posY;
        data.worldRow3[2] = entry.posZ;
        data.worldRow3[3] = 1.0f;
    }
    else
    {
        // Standardowa konwencja Direct3D (os Y skierowana w gore)
        data.worldRow0[0] = s * cosYaw;
        data.worldRow0[1] = 0.0f;
        data.worldRow0[2] = -s * sinYaw;
        data.worldRow0[3] = 0.0f;

        data.worldRow1[0] = 0.0f;
        data.worldRow1[1] = s;
        data.worldRow1[2] = 0.0f;
        data.worldRow1[3] = 0.0f;

        data.worldRow2[0] = s * sinYaw;
        data.worldRow2[1] = 0.0f;
        data.worldRow2[2] = s * cosYaw;
        data.worldRow2[3] = 0.0f;

        data.worldRow3[0] = entry.posX;
        data.worldRow3[1] = entry.posY;
        data.worldRow3[2] = entry.posZ;
        data.worldRow3[3] = 1.0f;
    }

    data.colorTint = entry.tintColor;
    data.windPhase = entry.windPhase;
    data.lodIndex = entry.lodIndex;
    data.padding = 0.0f;

    return data;
}

} // namespace GameLib
