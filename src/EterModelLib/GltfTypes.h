#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <initializer_list>

struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct float4 { float x, y, z, w; };
struct uint4 { unsigned int x, y, z, w; };
struct float4x4
{
    float m[4][4];

    float4x4() { std::memset(m, 0, sizeof(m)); }

    float4x4(std::initializer_list<float> list)
    {
        std::memset(m, 0, sizeof(m));
        auto it = list.begin();
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                if (it != list.end()) m[r][c] = *it++;
    }

    float4x4& operator=(std::initializer_list<float> list)
    {
        std::memset(m, 0, sizeof(m));
        auto it = list.begin();
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                if (it != list.end()) m[r][c] = *it++;
        return *this;
    }

    template <typename T>
    float4x4(const T& other)
    {
        std::memcpy(m, other.m, sizeof(m));
    }

    template <typename T>
    float4x4& operator=(const T& other)
    {
        std::memcpy(m, other.m, sizeof(m));
        return *this;
    }
};

struct GltfVertex
{
    float3 position;
    float3 normal;
    float2 uv0;
    float2 uv1;
    uint4 jointIndices;
    float4 jointWeights;
};

struct GltfSubmesh
{
    std::string name;
    int materialIndex;
    unsigned int vertexOffset;
    unsigned int vertexCount;
    unsigned int indexOffset;
    unsigned int indexCount;
};

struct GltfJoint
{
    std::string name;
    int parentIndex;
    float4x4 inverseBindMatrix;
    float3 localTranslation = {0.0f, 0.0f, 0.0f};
    float4 localRotation = {0.0f, 0.0f, 0.0f, 1.0f};
    float3 localScale = {1.0f, 1.0f, 1.0f};
};

struct GltfSkin
{
    std::string name;
    std::vector<GltfJoint> joints;
};

enum class GltfAnimationPathType
{
    Translation,
    Rotation,
    Scale,
    Unknown
};

struct GltfAnimationChannel
{
    int jointIndex;
    std::string targetNodeName;
    GltfAnimationPathType pathType;
    std::vector<float> times;
    std::vector<float4> values;
};

struct GltfEvent
{
    float time;
    std::string type;
    std::string arg;
};

struct GltfMotionData
{
    std::string name;
    float duration;
    std::vector<GltfAnimationChannel> channels;
    std::vector<GltfEvent> events;
};

struct GltfMaterial
{
    std::string name;
    std::string diffuseTexture;
    std::string opacityTexture;
    bool doubleSided = false;
};

struct GltfModelData
{
    std::string name;
    std::vector<GltfVertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<GltfSubmesh> submeshes;
    std::vector<GltfMaterial> materials;
    GltfSkin skin;
    std::vector<GltfMotionData> motions;
};

namespace EterModelLib
{
    using GltfVertex = ::GltfVertex;
    using GltfSubmesh = ::GltfSubmesh;
    using GltfJoint = ::GltfJoint;
    using GltfSkin = ::GltfSkin;
    using GltfAnimationPathType = ::GltfAnimationPathType;
    using GltfAnimationChannel = ::GltfAnimationChannel;
    using GltfEvent = ::GltfEvent;
    using GltfMotionData = ::GltfMotionData;
    using GltfMaterial = ::GltfMaterial;
    using GltfModelData = ::GltfModelData;
}

