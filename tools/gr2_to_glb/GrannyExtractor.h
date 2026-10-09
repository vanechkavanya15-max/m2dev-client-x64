#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <granny.h>

struct Metin2Event {
    float time;
    std::string type;
};

struct ExtractedVertex {
    float position[3];
    float normal[3];
    float uv[2];
    uint16_t joints[4];
    float weights[4];
};

struct ExtractedPrimitive {
    int materialIndex;
    std::vector<uint32_t> indices;
};

struct ExtractedMesh {
    std::string name;
    bool isRigid;
    int boneBindingCount;
    std::vector<ExtractedVertex> vertices;
    std::vector<ExtractedPrimitive> primitives;
};

struct ExtractedBone {
    std::string name;
    int parentIndex;
    float localTranslation[3];
    float localRotation[4]; // Quaternion (x, y, z, w)
    float localScale[3];
    float inverseBindMatrix[16]; // Kolumnowy uklad dla glTF 2.0
};

struct ExtractedSkeleton {
    std::string name;
    std::vector<ExtractedBone> bones;
};

struct ExtractedAnimationChannel {
    std::string boneName;
    std::vector<float> translations; // probki vec3
    std::vector<float> rotations;    // probki vec4 (quaternion)
    std::vector<float> scales;       // probki vec3
};

struct ExtractedAnimation {
    std::string name;
    float duration;
    float timeStep;
    std::vector<float> timeStamps;
    std::vector<ExtractedAnimationChannel> channels;
    std::vector<Metin2Event> events;
};

struct ExtractedMaterial {
    std::string name;
    std::string diffuseTexture;
    std::string opacityTexture;
};

#include <filesystem>

class GrannyExtractor {
public:
    GrannyExtractor();
    ~GrannyExtractor();

    bool Load(const std::filesystem::path& path);
    bool Load(const std::string& path);
    void Free();

    granny_file_info* GetFileInfo() const { return m_fileInfo; }
    const std::vector<Metin2Event>& GetEvents() const { return m_events; }

    const std::vector<ExtractedSkeleton>& GetSkeletons() const { return m_skeletons; }
    const std::vector<ExtractedMesh>& GetMeshes() const { return m_meshes; }
    const std::vector<ExtractedAnimation>& GetAnimations() const { return m_animations; }
    const std::vector<ExtractedMaterial>& GetMaterials() const { return m_materials; }

private:
    void ExtractSkeletons();
    void ExtractMeshes();
    void ExtractAnimations();
    void ExtractEvents();
    void ExtractMaterials();

    granny_file* m_file;
    granny_file_info* m_fileInfo;

    std::vector<Metin2Event> m_events;
    std::vector<ExtractedSkeleton> m_skeletons;
    std::vector<ExtractedMesh> m_meshes;
    std::vector<ExtractedAnimation> m_animations;
    std::vector<ExtractedMaterial> m_materials;
    std::vector<uint8_t> m_memoryBuffer;
};

