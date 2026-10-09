#include "GltfLoader.h"
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#include <iostream>
#include <cstring>

GltfLoader::GltfLoader() {}
GltfLoader::~GltfLoader() {}

bool GltfLoader::LoadFromFile(const std::string& path, GltfModelData& outModelData)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) return false;
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
    {
        cgltf_free(data);
        return false;
    }
    bool success = ProcessData(data, outModelData);
    cgltf_free(data);
    return success;
}

bool GltfLoader::LoadFromMemory(const void* bufferData, size_t size, GltfModelData& outModelData)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    if (cgltf_parse(&options, bufferData, size, &data) != cgltf_result_success) return false;
    if (cgltf_load_buffers(&options, data, nullptr) != cgltf_result_success)
    {
        cgltf_free(data);
        return false;
    }
    bool success = ProcessData(data, outModelData);
    cgltf_free(data);
    return success;
}

static std::string GetExtrasJsonString(const cgltf_data* data, const cgltf_extras& extras)
{
    if (extras.data && strlen(extras.data) > 0)
    {
        return std::string(extras.data);
    }
    if (extras.end_offset > extras.start_offset && data && data->json)
    {
        cgltf_size size = 0;
        if (cgltf_copy_extras_json(data, &extras, nullptr, &size) == cgltf_result_success && size > 1)
        {
            std::string buf(size, '\0');
            if (cgltf_copy_extras_json(data, &extras, &buf[0], &size) == cgltf_result_success)
            {
                if (!buf.empty() && buf.back() == '\0') buf.pop_back();
                return buf;
            }
        }
    }
    return "";
}

static void ParseMetin2Events(const std::string& json, std::vector<GltfEvent>& outEvents)
{
    size_t pos = json.find("\"metin2_events\"");
    if (pos == std::string::npos) return;

    size_t arrStart = json.find('[', pos);
    size_t arrEnd = json.find(']', arrStart);
    if (arrStart == std::string::npos || arrEnd == std::string::npos) return;

    size_t cur = arrStart + 1;
    while (cur < arrEnd)
    {
        size_t objStart = json.find('{', cur);
        if (objStart == std::string::npos || objStart >= arrEnd) break;
        size_t objEnd = json.find('}', objStart);
        if (objEnd == std::string::npos || objEnd > arrEnd) break;

        std::string objStr = json.substr(objStart, objEnd - objStart + 1);
        GltfEvent ev;
        ev.time = 0.0f;

        size_t timePos = objStr.find("\"time\"");
        if (timePos != std::string::npos)
        {
            size_t colon = objStr.find(':', timePos);
            if (colon != std::string::npos)
            {
                size_t numStart = objStr.find_first_not_of(" \t\r\n", colon + 1);
                if (numStart != std::string::npos)
                {
                    ev.time = (float)std::atof(objStr.c_str() + numStart);
                }
            }
        }

        size_t typePos = objStr.find("\"type\"");
        if (typePos != std::string::npos)
        {
            size_t colon = objStr.find(':', typePos);
            if (colon != std::string::npos)
            {
                size_t quoteStart = objStr.find('\"', colon + 1);
                if (quoteStart != std::string::npos)
                {
                    size_t quoteEnd = objStr.find('\"', quoteStart + 1);
                    if (quoteEnd != std::string::npos)
                    {
                        ev.type = objStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
            }
        }

        if (!ev.type.empty())
        {
            outEvents.push_back(ev);
        }

        cur = objEnd + 1;
    }
}

static void ParseMaterialExtras(const std::string& json, std::string& outDiffuse, std::string& outOpacity)
{
    auto extractString = [&](const std::string& key) -> std::string {
        size_t keyPos = json.find("\"" + key + "\"");
        if (keyPos == std::string::npos) return "";
        size_t colon = json.find(':', keyPos);
        if (colon == std::string::npos) return "";
        size_t q1 = json.find('\"', colon + 1);
        if (q1 == std::string::npos) return "";
        size_t q2 = json.find('\"', q1 + 1);
        if (q2 == std::string::npos) return "";
        return json.substr(q1 + 1, q2 - q1 - 1);
    };

    outDiffuse = extractString("diffuse_texture");
    outOpacity = extractString("opacity_texture");
}

void GltfLoader::ExtractEvents(cgltf_data* data, GltfModelData& outModelData)
{
    if (!data) return;

    for (size_t i = 0; i < data->animations_count && i < outModelData.motions.size(); ++i)
    {
        const cgltf_animation& anim = data->animations[i];
        std::string animExtras = GetExtrasJsonString(data, anim.extras);
        if (!animExtras.empty())
        {
            ParseMetin2Events(animExtras, outModelData.motions[i].events);
        }
    }

    std::string assetExtras = GetExtrasJsonString(data, data->asset.extras);
    if (!assetExtras.empty())
    {
        std::vector<GltfEvent> globalEvents;
        ParseMetin2Events(assetExtras, globalEvents);
        if (!globalEvents.empty())
        {
            for (auto& motion : outModelData.motions)
            {
                if (motion.events.empty())
                {
                    motion.events = globalEvents;
                }
            }
        }
    }
}

bool GltfLoader::ProcessData(cgltf_data* data, GltfModelData& outModelData)
{
    if (!data) return false;

    // Materialy
    for (size_t i = 0; i < data->materials_count; ++i)
    {
        const cgltf_material& mat = data->materials[i];
        GltfMaterial m;
        m.name = mat.name ? mat.name : "Material_" + std::to_string(i);
        m.doubleSided = mat.double_sided ? true : false;

        std::string matExtras = GetExtrasJsonString(data, mat.extras);
        if (!matExtras.empty())
        {
            ParseMaterialExtras(matExtras, m.diffuseTexture, m.opacityTexture);
        }
        outModelData.materials.push_back(m);
    }

    // Meshes (Geometria)
    for (size_t i = 0; i < data->meshes_count; ++i)
    {
        const cgltf_mesh& mesh = data->meshes[i];
        for (size_t j = 0; j < mesh.primitives_count; ++j)
        {
            const cgltf_primitive& prim = mesh.primitives[j];
            
            GltfSubmesh submesh;
            submesh.name = mesh.name ? mesh.name : "unnamed_mesh";
            submesh.materialIndex = prim.material ? (int)cgltf_material_index(data, prim.material) : -1;
            submesh.vertexOffset = (unsigned int)outModelData.vertices.size();
            submesh.indexOffset = (unsigned int)outModelData.indices.size();

            size_t vertexCount = 0;
            for (size_t k = 0; k < prim.attributes_count; ++k)
            {
                if (prim.attributes[k].data->count > vertexCount)
                    vertexCount = prim.attributes[k].data->count;
            }
            submesh.vertexCount = (unsigned int)vertexCount;

            size_t startVertex = outModelData.vertices.size();
            outModelData.vertices.resize(startVertex + vertexCount);

            // Wyciaganie atrybutow z bounds-checking
            for (size_t k = 0; k < prim.attributes_count; ++k)
            {
                const cgltf_attribute& attr = prim.attributes[k];
                for (size_t v = 0; v < attr.data->count; ++v)
                {
                    GltfVertex& vertex = outModelData.vertices[startVertex + v];
                    float values[4] = {0};
                    cgltf_accessor_read_float(attr.data, v, values, 4);

                    if (attr.type == cgltf_attribute_type_position) {
                        vertex.position = {values[0], values[1], values[2]};
                    } else if (attr.type == cgltf_attribute_type_normal) {
                        vertex.normal = {values[0], values[1], values[2]};
                    } else if (attr.type == cgltf_attribute_type_texcoord) {
                        if (attr.index == 0) vertex.uv0 = {values[0], values[1]};
                        else if (attr.index == 1) vertex.uv1 = {values[0], values[1]};
                    } else if (attr.type == cgltf_attribute_type_joints) {
                        vertex.jointIndices = {(unsigned int)values[0], (unsigned int)values[1], (unsigned int)values[2], (unsigned int)values[3]};
                    } else if (attr.type == cgltf_attribute_type_weights) {
                        vertex.jointWeights = {values[0], values[1], values[2], values[3]};
                    }
                }
            }

            // Indeksy (Index buffery z poprawnym offsetem wierzcholkow submesha)
            if (prim.indices)
            {
                submesh.indexCount = (unsigned int)prim.indices->count;
                size_t startIndex = outModelData.indices.size();
                outModelData.indices.resize(startIndex + submesh.indexCount);
                for (size_t idx = 0; idx < prim.indices->count; ++idx)
                {
                    outModelData.indices[startIndex + idx] = (unsigned int)cgltf_accessor_read_index(prim.indices, idx) + (unsigned int)startVertex;
                }
            }
            else
            {
                submesh.indexCount = (unsigned int)vertexCount;
                size_t startIndex = outModelData.indices.size();
                outModelData.indices.resize(startIndex + submesh.indexCount);
                for (size_t idx = 0; idx < vertexCount; ++idx)
                {
                    outModelData.indices[startIndex + idx] = (unsigned int)(startVertex + idx);
                }
            }

            outModelData.submeshes.push_back(submesh);
        }
    }

    // Szkielet (Skin / Joints / InverseBindMatrices / Rest Pose)
    if (data->skins_count > 0)
    {
        const cgltf_skin& skin = data->skins[0];
        outModelData.skin.name = skin.name ? skin.name : "unnamed_skin";
        
        std::vector<float> invBindMatrices(skin.joints_count * 16);
        if (skin.inverse_bind_matrices) {
            for (size_t i = 0; i < skin.joints_count; ++i) {
                cgltf_accessor_read_float(skin.inverse_bind_matrices, i, &invBindMatrices[i * 16], 16);
            }
        }

        for (size_t i = 0; i < skin.joints_count; ++i)
        {
            const cgltf_node* jointNode = skin.joints[i];
            GltfJoint joint;
            joint.name = jointNode->name ? jointNode->name : "unnamed_joint";
            joint.parentIndex = -1;
            
            if (jointNode->parent) {
                for (size_t p = 0; p < skin.joints_count; ++p) {
                    if (skin.joints[p] == jointNode->parent) {
                        joint.parentIndex = (int)p;
                        break;
                    }
                }
            }

            // Odczyt bind pose (rest pose) kosci
            if (jointNode->has_translation) {
                joint.localTranslation = {jointNode->translation[0], jointNode->translation[1], jointNode->translation[2]};
            }
            if (jointNode->has_rotation) {
                joint.localRotation = {jointNode->rotation[0], jointNode->rotation[1], jointNode->rotation[2], jointNode->rotation[3]};
            }
            if (jointNode->has_scale) {
                joint.localScale = {jointNode->scale[0], jointNode->scale[1], jointNode->scale[2]};
            }
            
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    joint.inverseBindMatrix.m[r][c] = invBindMatrices[i * 16 + r * 4 + c];
                }
            }
            outModelData.skin.joints.push_back(joint);
        }
    }

    // Animacje
    for (size_t i = 0; i < data->animations_count; ++i)
    {
        const cgltf_animation& anim = data->animations[i];
        GltfMotionData motion;
        motion.name = anim.name ? anim.name : "unnamed_animation";
        motion.duration = 0.0f;

        for (size_t c = 0; c < anim.channels_count; ++c)
        {
            const cgltf_animation_channel& channel = anim.channels[c];
            GltfAnimationChannel animChannel;
            
            animChannel.jointIndex = -1;
            if (channel.target_node && channel.target_node->name) {
                animChannel.targetNodeName = channel.target_node->name;
            }

            if (data->skins_count > 0) {
                const cgltf_skin& skin = data->skins[0];
                for (size_t j = 0; j < skin.joints_count; ++j) {
                    if (skin.joints[j] == channel.target_node) {
                        animChannel.jointIndex = (int)j;
                        break;
                    }
                }
            }
            
            if (channel.target_path == cgltf_animation_path_type_translation) animChannel.pathType = GltfAnimationPathType::Translation;
            else if (channel.target_path == cgltf_animation_path_type_rotation) animChannel.pathType = GltfAnimationPathType::Rotation;
            else if (channel.target_path == cgltf_animation_path_type_scale) animChannel.pathType = GltfAnimationPathType::Scale;
            else animChannel.pathType = GltfAnimationPathType::Unknown;
            
            const cgltf_animation_sampler* sampler = channel.sampler;
            if (sampler && sampler->input && sampler->output)
            {
                size_t inputCount = sampler->input->count;
                animChannel.times.resize(inputCount);
                for (size_t t = 0; t < inputCount; ++t) {
                    float time = 0;
                    cgltf_accessor_read_float(sampler->input, t, &time, 1);
                    animChannel.times[t] = time;
                    if (time > motion.duration) motion.duration = time;
                }
                
                size_t outputCount = sampler->output->count;
                animChannel.values.resize(outputCount);
                for (size_t v = 0; v < outputCount; ++v) {
                    float val[4] = {0,0,0,0};
                    size_t numComps = cgltf_num_components(sampler->output->type);
                    cgltf_accessor_read_float(sampler->output, v, val, numComps);
                    animChannel.values[v] = {val[0], val[1], val[2], val[3]};
                }
            }
            motion.channels.push_back(animChannel);
        }
        outModelData.motions.push_back(motion);
    }
    
    // Metin2 Events (Extract)
    ExtractEvents(data, outModelData);

    return true;
}
