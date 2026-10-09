#pragma once
#include <string>
#include "GrannyExtractor.h"
#include <cgltf/cgltf_write.h>

#include <filesystem>

class GlbWriter {
public:
    bool Write(const std::filesystem::path& outputPath, const GrannyExtractor& extractor);
    bool Write(const std::string& outputPath, const GrannyExtractor& extractor);

private:
    std::string BuildExtrasJson(const std::vector<Metin2Event>& events);
    std::string BuildMaterialExtrasJson(const ExtractedMaterial& mat);
};
