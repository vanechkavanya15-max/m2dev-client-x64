#pragma once

#include "GltfTypes.h"
#include <string>

class GltfLoader
{
public:
    GltfLoader();
    ~GltfLoader();

    bool LoadFromFile(const std::string& path, GltfModelData& outModelData);
    bool LoadFromMemory(const void* data, size_t size, GltfModelData& outModelData);

private:
    bool ProcessData(struct cgltf_data* data, GltfModelData& outModelData);
    void ExtractEvents(struct cgltf_data* data, GltfModelData& outModelData);
};
