#pragma once

#include "GltfTypes.h"
#include <string>
#include <vector>

namespace EterModelLib
{

class CGltfModel
{
public:
    CGltfModel();
    virtual ~CGltfModel();

    // Ladowanie z pliku oraz z pamieci
    bool LoadFromFile(const std::string& filename);
    bool LoadFromMemory(const void* data, size_t size, const std::string& modelName = "");

    // Czyszczenie
    void Clear();
    bool IsLoaded() const { return m_bLoaded || !m_modelData.vertices.empty() || !m_modelData.submeshes.empty() || !m_modelData.skin.joints.empty(); }
    void SetLoaded(bool loaded = true) { m_bLoaded = loaded; }

    // Nazwa modelu
    const std::string& GetName() const { return m_modelData.name; }

    // Dostep do podsiatek (Submeshes)
    size_t GetSubmeshCount() const { return m_modelData.submeshes.size(); }
    const GltfSubmesh* GetSubmesh(size_t index) const;
    const std::vector<GltfSubmesh>& GetSubmeshes() const { return m_modelData.submeshes; }

    // Dostep do kosci i szkieletu (Joints / Skin)
    size_t GetBoneCount() const { return m_modelData.skin.joints.size(); }
    const GltfJoint* GetBone(size_t index) const;
    int FindBoneIndex(const std::string& name) const;
    const GltfSkin& GetSkin() const { return m_modelData.skin; }

    // Dostep do animacji (Motions / Animations)
    size_t GetAnimationCount() const { return m_modelData.motions.size(); }
    const GltfMotionData* GetAnimation(size_t index) const;
    const GltfMotionData* FindAnimation(const std::string& name) const;
    int FindAnimationIndex(const std::string& name) const;
    const std::vector<GltfMotionData>& GetAnimations() const { return m_modelData.motions; }

    // Dostep do geometrii wierzcholkow i indeksow
    size_t GetVertexCount() const { return m_modelData.vertices.size(); }
    size_t GetIndexCount() const { return m_modelData.indices.size(); }
    const std::vector<GltfVertex>& GetVertices() const { return m_modelData.vertices; }
    const std::vector<unsigned int>& GetIndices() const { return m_modelData.indices; }

    // Pelna struktura modelu
    const GltfModelData& GetModelData() const { return m_modelData; }
    GltfModelData& GetModelData() { return m_modelData; }

private:
    bool m_bLoaded;
    GltfModelData m_modelData;
};

} // namespace EterModelLib

using CGltfModel = EterModelLib::CGltfModel;
