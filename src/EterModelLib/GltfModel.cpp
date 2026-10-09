#include "GltfModel.h"
#include "GltfLoader.h"

namespace EterModelLib
{

CGltfModel::CGltfModel()
    : m_bLoaded(false)
{
}

CGltfModel::~CGltfModel()
{
    Clear();
}

void CGltfModel::Clear()
{
    m_bLoaded = false;
    m_modelData.name.clear();
    m_modelData.vertices.clear();
    m_modelData.indices.clear();
    m_modelData.submeshes.clear();
    m_modelData.skin.name.clear();
    m_modelData.skin.joints.clear();
    m_modelData.motions.clear();
}

bool CGltfModel::LoadFromFile(const std::string& filename)
{
    Clear();
    GltfLoader loader;
    if (!loader.LoadFromFile(filename, m_modelData))
    {
        return false;
    }

    if (m_modelData.name.empty())
    {
        m_modelData.name = filename;
    }

    m_bLoaded = true;
    return true;
}

bool CGltfModel::LoadFromMemory(const void* data, size_t size, const std::string& modelName)
{
    Clear();
    GltfLoader loader;
    if (!loader.LoadFromMemory(data, size, m_modelData))
    {
        return false;
    }

    if (!modelName.empty())
    {
        m_modelData.name = modelName;
    }

    m_bLoaded = true;
    return true;
}

const GltfSubmesh* CGltfModel::GetSubmesh(size_t index) const
{
    if (index < m_modelData.submeshes.size())
    {
        return &m_modelData.submeshes[index];
    }
    return nullptr;
}

const GltfJoint* CGltfModel::GetBone(size_t index) const
{
    if (index < m_modelData.skin.joints.size())
    {
        return &m_modelData.skin.joints[index];
    }
    return nullptr;
}

int CGltfModel::FindBoneIndex(const std::string& name) const
{
    for (size_t i = 0; i < m_modelData.skin.joints.size(); ++i)
    {
        if (m_modelData.skin.joints[i].name == name)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

const GltfMotionData* CGltfModel::GetAnimation(size_t index) const
{
    if (index < m_modelData.motions.size())
    {
        return &m_modelData.motions[index];
    }
    return nullptr;
}

int CGltfModel::FindAnimationIndex(const std::string& name) const
{
    for (size_t i = 0; i < m_modelData.motions.size(); ++i)
    {
        if (m_modelData.motions[i].name == name)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

const GltfMotionData* CGltfModel::FindAnimation(const std::string& name) const
{
    int index = FindAnimationIndex(name);
    if (index >= 0)
    {
        return &m_modelData.motions[index];
    }
    return nullptr;
}

} // namespace EterModelLib
