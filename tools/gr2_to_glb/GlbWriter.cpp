#include "GlbWriter.h"
#define CGLTF_WRITE_IMPLEMENTATION
#include <cgltf/cgltf_write.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <cfloat>
#include <cmath>
#include <cstring>

bool GlbWriter::Write(const std::string& outputPath, const GrannyExtractor& extractor) {
    granny_file_info* fileInfo = extractor.GetFileInfo();
    if (!fileInfo) {
        std::cerr << "Brak danych z pliku Granny." << std::endl;
        return false;
    }

    std::vector<uint8_t> binBuffer;
    auto appendBin = [&](const void* ptr, size_t size, size_t alignment = 4) -> size_t {
        size_t current = binBuffer.size();
        size_t pad = (alignment - (current % alignment)) % alignment;
        if (pad > 0) {
            binBuffer.insert(binBuffer.end(), pad, 0);
        }
        size_t offset = binBuffer.size();
        const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(ptr);
        binBuffer.insert(binBuffer.end(), bytePtr, bytePtr + size);
        return offset;
    };

    std::vector<cgltf_buffer_view> bufferViews;
    std::vector<cgltf_accessor> accessors;

    const auto& extractedSkeletons = extractor.GetSkeletons();
    const auto& extractedMeshes = extractor.GetMeshes();
    const auto& extractedAnims = extractor.GetAnimations();
    const auto& extractedMaterials = extractor.GetMaterials();

    bool hasSkeleton = !extractedSkeletons.empty() && !extractedSkeletons[0].bones.empty();
    size_t boneCount = 0;
    std::map<std::string, size_t> boneNameToNodeIndex;

    if (hasSkeleton) {
        boneCount = extractedSkeletons[0].bones.size();
        for (size_t b = 0; b < boneCount; ++b) {
            boneNameToNodeIndex[extractedSkeletons[0].bones[b].name] = b;
        }
    } else {
        // Jesli nie ma szkieletu, zbierz unikalne kosci z animacji
        std::vector<std::string> animBoneNames;
        for (const auto& anim : extractedAnims) {
            for (const auto& chan : anim.channels) {
                if (boneNameToNodeIndex.find(chan.boneName) == boneNameToNodeIndex.end()) {
                    boneNameToNodeIndex[chan.boneName] = animBoneNames.size();
                    animBoneNames.push_back(chan.boneName);
                }
            }
        }
        boneCount = animBoneNames.size();
    }

    size_t meshCount = extractedMeshes.size();
    size_t totalNodeCount = boneCount + meshCount;

    std::vector<cgltf_node> nodes(totalNodeCount);
    std::vector<std::vector<cgltf_node*>> nodeChildren(totalNodeCount);
    std::vector<cgltf_node*> sceneNodes;

    // Tworzenie wezlow kosci
    if (hasSkeleton) {
        const auto& skel = extractedSkeletons[0];
        for (size_t b = 0; b < boneCount; ++b) {
            const auto& eb = skel.bones[b];
            cgltf_node& n = nodes[b];
            n.name = _strdup(eb.name.c_str());
            n.has_translation = 1;
            n.translation[0] = eb.localTranslation[0];
            n.translation[1] = eb.localTranslation[1];
            n.translation[2] = eb.localTranslation[2];

            n.has_rotation = 1;
            n.rotation[0] = eb.localRotation[0];
            n.rotation[1] = eb.localRotation[1];
            n.rotation[2] = eb.localRotation[2];
            n.rotation[3] = eb.localRotation[3];

            n.has_scale = 1;
            n.scale[0] = eb.localScale[0];
            n.scale[1] = eb.localScale[1];
            n.scale[2] = eb.localScale[2];

            if (eb.parentIndex >= 0 && (size_t)eb.parentIndex < boneCount) {
                nodeChildren[eb.parentIndex].push_back(&nodes[b]);
                n.parent = &nodes[eb.parentIndex];
            } else {
                sceneNodes.push_back(&nodes[b]);
            }
        }
    } else {
        // Animacje bez szkieletu - wezly z nazw trackow
        for (const auto& pair : boneNameToNodeIndex) {
            size_t b = pair.second;
            cgltf_node& n = nodes[b];
            n.name = _strdup(pair.first.c_str());
            sceneNodes.push_back(&nodes[b]);
        }
    }

    // Tworzenie bufora InverseBindMatrices dla szkieletu
    size_t ibmAccIndex = 0;
    if (hasSkeleton) {
        const auto& skel = extractedSkeletons[0];
        std::vector<float> ibmData(boneCount * 16);
        for (size_t b = 0; b < boneCount; ++b) {
            memcpy(&ibmData[b * 16], skel.bones[b].inverseBindMatrix, 16 * sizeof(float));
        }
        size_t ibmOffset = appendBin(ibmData.data(), ibmData.size() * sizeof(float));

        cgltf_buffer_view ibmView = {};
        ibmView.offset = ibmOffset;
        ibmView.size = ibmData.size() * sizeof(float);
        bufferViews.push_back(ibmView);

        cgltf_accessor ibmAcc = {};
        ibmAcc.buffer_view = nullptr; // powiazemy po ustabilizowaniu vectora
        ibmAcc.type = cgltf_type_mat4;
        ibmAcc.component_type = cgltf_component_type_r_32f;
        ibmAcc.count = boneCount;
        ibmAccIndex = accessors.size();
        accessors.push_back(ibmAcc);
    }

    // Materialy
    std::vector<cgltf_material> materials(extractedMaterials.size());
    for (size_t i = 0; i < materials.size(); ++i) {
        materials[i].name = _strdup(extractedMaterials[i].name.c_str());
        materials[i].has_pbr_metallic_roughness = 1;
        materials[i].pbr_metallic_roughness.metallic_factor = 0.0f;
        materials[i].pbr_metallic_roughness.roughness_factor = 0.8f;
        materials[i].pbr_metallic_roughness.base_color_factor[0] = 1.0f;
        materials[i].pbr_metallic_roughness.base_color_factor[1] = 1.0f;
        materials[i].pbr_metallic_roughness.base_color_factor[2] = 1.0f;
        materials[i].pbr_metallic_roughness.base_color_factor[3] = 1.0f;

        std::string matJson = BuildMaterialExtrasJson(extractedMaterials[i]);
        if (!matJson.empty()) {
            materials[i].extras.data = _strdup(matJson.c_str());
        }
    }

    // Siatki (Meshes)
    std::vector<cgltf_mesh> meshes(meshCount);
    std::vector<std::vector<cgltf_primitive>> meshPrimitives(meshCount);
    std::vector<std::vector<std::vector<cgltf_attribute>>> primAttributes(meshCount);

    struct MeshAccessorIndices {
        size_t posAccIdx;
        size_t normAccIdx;
        size_t uvAccIdx;
        size_t jointsAccIdx;
        size_t weightsAccIdx;
        std::vector<size_t> primIndicesAccIdx;
    };
    std::vector<MeshAccessorIndices> meshAccIndices(meshCount);

    for (size_t m = 0; m < meshCount; ++m) {
        const auto& extMesh = extractedMeshes[m];
        meshes[m].name = _strdup(extMesh.name.c_str());
        size_t vCount = extMesh.vertices.size();

        // 1. POSITION
        std::vector<float> posData(vCount * 3);
        float minPos[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
        float maxPos[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        for (size_t v = 0; v < vCount; ++v) {
            posData[v * 3 + 0] = extMesh.vertices[v].position[0];
            posData[v * 3 + 1] = extMesh.vertices[v].position[1];
            posData[v * 3 + 2] = extMesh.vertices[v].position[2];
            for (int c = 0; c < 3; ++c) {
                if (posData[v * 3 + c] < minPos[c]) minPos[c] = posData[v * 3 + c];
                if (posData[v * 3 + c] > maxPos[c]) maxPos[c] = posData[v * 3 + c];
            }
        }
        size_t posOffset = appendBin(posData.data(), posData.size() * sizeof(float));
        cgltf_buffer_view posView = {};
        posView.offset = posOffset;
        posView.size = posData.size() * sizeof(float);
        posView.type = cgltf_buffer_view_type_vertices;
        size_t posViewIdx = bufferViews.size();
        bufferViews.push_back(posView);

        cgltf_accessor posAcc = {};
        posAcc.type = cgltf_type_vec3;
        posAcc.component_type = cgltf_component_type_r_32f;
        posAcc.count = vCount;
        posAcc.has_min = 1;
        posAcc.min[0] = minPos[0]; posAcc.min[1] = minPos[1]; posAcc.min[2] = minPos[2];
        posAcc.has_max = 1;
        posAcc.max[0] = maxPos[0]; posAcc.max[1] = maxPos[1]; posAcc.max[2] = maxPos[2];
        meshAccIndices[m].posAccIdx = accessors.size();
        accessors.push_back(posAcc);

        // 2. NORMAL
        std::vector<float> normData(vCount * 3);
        for (size_t v = 0; v < vCount; ++v) {
            normData[v * 3 + 0] = extMesh.vertices[v].normal[0];
            normData[v * 3 + 1] = extMesh.vertices[v].normal[1];
            normData[v * 3 + 2] = extMesh.vertices[v].normal[2];
        }
        size_t normOffset = appendBin(normData.data(), normData.size() * sizeof(float));
        cgltf_buffer_view normView = {};
        normView.offset = normOffset;
        normView.size = normData.size() * sizeof(float);
        normView.type = cgltf_buffer_view_type_vertices;
        size_t normViewIdx = bufferViews.size();
        bufferViews.push_back(normView);

        cgltf_accessor normAcc = {};
        normAcc.type = cgltf_type_vec3;
        normAcc.component_type = cgltf_component_type_r_32f;
        normAcc.count = vCount;
        meshAccIndices[m].normAccIdx = accessors.size();
        accessors.push_back(normAcc);

        // 3. TEXCOORD_0
        std::vector<float> uvData(vCount * 2);
        for (size_t v = 0; v < vCount; ++v) {
            uvData[v * 2 + 0] = extMesh.vertices[v].uv[0];
            uvData[v * 2 + 1] = extMesh.vertices[v].uv[1];
        }
        size_t uvOffset = appendBin(uvData.data(), uvData.size() * sizeof(float));
        cgltf_buffer_view uvView = {};
        uvView.offset = uvOffset;
        uvView.size = uvData.size() * sizeof(float);
        uvView.type = cgltf_buffer_view_type_vertices;
        size_t uvViewIdx = bufferViews.size();
        bufferViews.push_back(uvView);

        cgltf_accessor uvAcc = {};
        uvAcc.type = cgltf_type_vec2;
        uvAcc.component_type = cgltf_component_type_r_32f;
        uvAcc.count = vCount;
        meshAccIndices[m].uvAccIdx = accessors.size();
        accessors.push_back(uvAcc);

        // 4. JOINTS_0 & WEIGHTS_0 (dla deformowanych siatek szkieletowych)
        if (hasSkeleton) {
            std::vector<uint16_t> jointsData(vCount * 4);
            std::vector<float> weightsData(vCount * 4);
            for (size_t v = 0; v < vCount; ++v) {
                for (int k = 0; k < 4; ++k) {
                    jointsData[v * 4 + k] = extMesh.vertices[v].joints[k];
                    weightsData[v * 4 + k] = extMesh.vertices[v].weights[k];
                }
            }

            size_t jointsOffset = appendBin(jointsData.data(), jointsData.size() * sizeof(uint16_t));
            cgltf_buffer_view jointsView = {};
            jointsView.offset = jointsOffset;
            jointsView.size = jointsData.size() * sizeof(uint16_t);
            jointsView.type = cgltf_buffer_view_type_vertices;
            size_t jointsViewIdx = bufferViews.size();
            bufferViews.push_back(jointsView);

            cgltf_accessor jointsAcc = {};
            jointsAcc.type = cgltf_type_vec4;
            jointsAcc.component_type = cgltf_component_type_r_16u;
            jointsAcc.count = vCount;
            meshAccIndices[m].jointsAccIdx = accessors.size();
            accessors.push_back(jointsAcc);

            size_t weightsOffset = appendBin(weightsData.data(), weightsData.size() * sizeof(float));
            cgltf_buffer_view weightsView = {};
            weightsView.offset = weightsOffset;
            weightsView.size = weightsData.size() * sizeof(float);
            weightsView.type = cgltf_buffer_view_type_vertices;
            size_t weightsViewIdx = bufferViews.size();
            bufferViews.push_back(weightsView);

            cgltf_accessor weightsAcc = {};
            weightsAcc.type = cgltf_type_vec4;
            weightsAcc.component_type = cgltf_component_type_r_32f;
            weightsAcc.count = vCount;
            meshAccIndices[m].weightsAccIdx = accessors.size();
            accessors.push_back(weightsAcc);
        }

        // 5. INDICES dla kazdego primitive
        size_t primCount = extMesh.primitives.size();
        meshAccIndices[m].primIndicesAccIdx.resize(primCount);
        for (size_t p = 0; p < primCount; ++p) {
            const auto& extPrim = extMesh.primitives[p];
            size_t idxCount = extPrim.indices.size();

            if (vCount <= 65536) {
                std::vector<uint16_t> idx16(idxCount);
                for (size_t i = 0; i < idxCount; ++i) {
                    idx16[i] = (uint16_t)extPrim.indices[i];
                }
                size_t idxOffset = appendBin(idx16.data(), idx16.size() * sizeof(uint16_t));
                cgltf_buffer_view idxView = {};
                idxView.offset = idxOffset;
                idxView.size = idx16.size() * sizeof(uint16_t);
                idxView.type = cgltf_buffer_view_type_indices;
                size_t idxViewIdx = bufferViews.size();
                bufferViews.push_back(idxView);

                cgltf_accessor idxAcc = {};
                idxAcc.type = cgltf_type_scalar;
                idxAcc.component_type = cgltf_component_type_r_16u;
                idxAcc.count = idxCount;
                meshAccIndices[m].primIndicesAccIdx[p] = accessors.size();
                accessors.push_back(idxAcc);
            } else {
                size_t idxOffset = appendBin(extPrim.indices.data(), extPrim.indices.size() * sizeof(uint32_t));
                cgltf_buffer_view idxView = {};
                idxView.offset = idxOffset;
                idxView.size = extPrim.indices.size() * sizeof(uint32_t);
                idxView.type = cgltf_buffer_view_type_indices;
                size_t idxViewIdx = bufferViews.size();
                bufferViews.push_back(idxView);

                cgltf_accessor idxAcc = {};
                idxAcc.type = cgltf_type_scalar;
                idxAcc.component_type = cgltf_component_type_r_32u;
                idxAcc.count = idxCount;
                meshAccIndices[m].primIndicesAccIdx[p] = accessors.size();
                accessors.push_back(idxAcc);
            }
        }
    }

    // Animacje
    size_t animCount = extractedAnims.size();
    std::vector<cgltf_animation> animations(animCount);
    std::vector<std::vector<cgltf_animation_sampler>> animSamplers(animCount);
    std::vector<std::vector<cgltf_animation_channel>> animChannels(animCount);

    struct AnimChannelMeta {
        size_t nodeIdx;
        cgltf_animation_path_type pathType;
        size_t timeAccIdx;
        size_t dataAccIdx;
    };
    std::vector<std::vector<AnimChannelMeta>> animChannelMetas(animCount);

    for (size_t a = 0; a < animCount; ++a) {
        const auto& extAnim = extractedAnims[a];
        animations[a].name = _strdup(extAnim.name.c_str());

        size_t sampleCount = extAnim.timeStamps.size();
        size_t timeOffset = appendBin(extAnim.timeStamps.data(), sampleCount * sizeof(float));

        cgltf_buffer_view timeView = {};
        timeView.offset = timeOffset;
        timeView.size = sampleCount * sizeof(float);
        size_t timeViewIdx = bufferViews.size();
        bufferViews.push_back(timeView);

        cgltf_accessor timeAcc = {};
        timeAcc.type = cgltf_type_scalar;
        timeAcc.component_type = cgltf_component_type_r_32f;
        timeAcc.count = sampleCount;
        timeAcc.has_min = 1;
        timeAcc.min[0] = extAnim.timeStamps.front();
        timeAcc.has_max = 1;
        timeAcc.max[0] = extAnim.timeStamps.back();
        size_t timeAccIdx = accessors.size();
        accessors.push_back(timeAcc);

        for (const auto& chan : extAnim.channels) {
            auto it = boneNameToNodeIndex.find(chan.boneName);
            if (it == boneNameToNodeIndex.end()) continue;
            size_t targetNodeIdx = it->second;

            // 1. Translacja
            if (chan.translations.size() == sampleCount * 3) {
                size_t tOffset = appendBin(chan.translations.data(), chan.translations.size() * sizeof(float));
                cgltf_buffer_view tView = {};
                tView.offset = tOffset;
                tView.size = chan.translations.size() * sizeof(float);
                size_t tViewIdx = bufferViews.size();
                bufferViews.push_back(tView);

                cgltf_accessor tAcc = {};
                tAcc.type = cgltf_type_vec3;
                tAcc.component_type = cgltf_component_type_r_32f;
                tAcc.count = sampleCount;
                size_t tAccIdx = accessors.size();
                accessors.push_back(tAcc);

                AnimChannelMeta meta;
                meta.nodeIdx = targetNodeIdx;
                meta.pathType = cgltf_animation_path_type_translation;
                meta.timeAccIdx = timeAccIdx;
                meta.dataAccIdx = tAccIdx;
                animChannelMetas[a].push_back(meta);
            }

            // 2. Rotacja
            if (chan.rotations.size() == sampleCount * 4) {
                size_t rOffset = appendBin(chan.rotations.data(), chan.rotations.size() * sizeof(float));
                cgltf_buffer_view rView = {};
                rView.offset = rOffset;
                rView.size = chan.rotations.size() * sizeof(float);
                size_t rViewIdx = bufferViews.size();
                bufferViews.push_back(rView);

                cgltf_accessor rAcc = {};
                rAcc.type = cgltf_type_vec4;
                rAcc.component_type = cgltf_component_type_r_32f;
                rAcc.count = sampleCount;
                size_t rAccIdx = accessors.size();
                accessors.push_back(rAcc);

                AnimChannelMeta meta;
                meta.nodeIdx = targetNodeIdx;
                meta.pathType = cgltf_animation_path_type_rotation;
                meta.timeAccIdx = timeAccIdx;
                meta.dataAccIdx = rAccIdx;
                animChannelMetas[a].push_back(meta);
            }

            // 3. Skala
            if (chan.scales.size() == sampleCount * 3) {
                bool hasScaleAnim = false;
                for (size_t s = 0; s < chan.scales.size(); ++s) {
                    if (std::abs(chan.scales[s] - 1.0f) > 1e-4f) {
                        hasScaleAnim = true;
                        break;
                    }
                }
                if (hasScaleAnim) {
                    size_t sOffset = appendBin(chan.scales.data(), chan.scales.size() * sizeof(float));
                    cgltf_buffer_view sView = {};
                    sView.offset = sOffset;
                    sView.size = chan.scales.size() * sizeof(float);
                    size_t sViewIdx = bufferViews.size();
                    bufferViews.push_back(sView);

                    cgltf_accessor sAcc = {};
                    sAcc.type = cgltf_type_vec3;
                    sAcc.component_type = cgltf_component_type_r_32f;
                    sAcc.count = sampleCount;
                    size_t sAccIdx = accessors.size();
                    accessors.push_back(sAcc);

                    AnimChannelMeta meta;
                    meta.nodeIdx = targetNodeIdx;
                    meta.pathType = cgltf_animation_path_type_scale;
                    meta.timeAccIdx = timeAccIdx;
                    meta.dataAccIdx = sAccIdx;
                    animChannelMetas[a].push_back(meta);
                }
            }
        }
    }

    // Zaokraglenie bufora do wielokrotnosci 4 bajtow
    while (binBuffer.size() % 4 != 0) {
        binBuffer.push_back(0);
    }

    // Definicja glownego bufora cgltf_buffer
    std::vector<cgltf_buffer> buffers(1);
    buffers[0].name = _strdup("binary_buffer");
    buffers[0].size = binBuffer.size();

    // Polaczenie buforow i buffer_views
    for (size_t i = 0; i < bufferViews.size(); ++i) {
        bufferViews[i].buffer = &buffers[0];
    }

    // Polaczenie accessorow z buffer_views
    // Kazdy accessor odpowiada swojemu bufferView 1:1
    for (size_t i = 0; i < accessors.size(); ++i) {
        accessors[i].buffer_view = &bufferViews[i];
    }

    // Skonstruowanie Skin
    std::vector<cgltf_skin> skins;
    std::vector<std::vector<cgltf_node*>> skinJoints;
    if (hasSkeleton) {
        skins.resize(1);
        skinJoints.resize(1);
        skinJoints[0].resize(boneCount);
        for (size_t b = 0; b < boneCount; ++b) {
            skinJoints[0][b] = &nodes[b];
        }
        skins[0].name = _strdup(extractedSkeletons[0].name.c_str());
        skins[0].joints_count = boneCount;
        skins[0].joints = skinJoints[0].data();
        skins[0].skeleton = &nodes[0];
        skins[0].inverse_bind_matrices = &accessors[ibmAccIndex];
    }

    // Polaczenie wezlow siatek (Mesh Nodes)
    for (size_t m = 0; m < meshCount; ++m) {
        size_t nodeIdx = boneCount + m;
        cgltf_node& n = nodes[nodeIdx];
        n.name = _strdup(extractedMeshes[m].name.c_str());
        n.mesh = &meshes[m];
        if (hasSkeleton) {
            n.skin = &skins[0];
        }
        sceneNodes.push_back(&nodes[nodeIdx]);
    }

    // Polaczenie hierarchii node children
    for (size_t i = 0; i < totalNodeCount; ++i) {
        nodes[i].children_count = nodeChildren[i].size();
        nodes[i].children = nodeChildren[i].empty() ? nullptr : nodeChildren[i].data();
    }

    // Polaczenie Meshes z Primtives i Accessors
    for (size_t m = 0; m < meshCount; ++m) {
        const auto& extMesh = extractedMeshes[m];
        size_t primCount = extMesh.primitives.size();
        meshPrimitives[m].resize(primCount);
        primAttributes[m].resize(primCount);

        int attrCount = hasSkeleton ? 5 : 3;

        for (size_t p = 0; p < primCount; ++p) {
            const auto& extPrim = extMesh.primitives[p];
            primAttributes[m][p].resize(attrCount);
            auto& attrs = primAttributes[m][p];

            attrs[0].type = cgltf_attribute_type_position;
            attrs[0].name = (char*)"POSITION";
            attrs[0].data = &accessors[meshAccIndices[m].posAccIdx];

            attrs[1].type = cgltf_attribute_type_normal;
            attrs[1].name = (char*)"NORMAL";
            attrs[1].data = &accessors[meshAccIndices[m].normAccIdx];

            attrs[2].type = cgltf_attribute_type_texcoord;
            attrs[2].name = (char*)"TEXCOORD_0";
            attrs[2].data = &accessors[meshAccIndices[m].uvAccIdx];

            if (hasSkeleton) {
                attrs[3].type = cgltf_attribute_type_joints;
                attrs[3].name = (char*)"JOINTS_0";
                attrs[3].data = &accessors[meshAccIndices[m].jointsAccIdx];

                attrs[4].type = cgltf_attribute_type_weights;
                attrs[4].name = (char*)"WEIGHTS_0";
                attrs[4].data = &accessors[meshAccIndices[m].weightsAccIdx];
            }

            cgltf_primitive& prim = meshPrimitives[m][p];
            prim.type = cgltf_primitive_type_triangles;
            prim.attributes_count = attrs.size();
            prim.attributes = attrs.data();
            prim.indices = &accessors[meshAccIndices[m].primIndicesAccIdx[p]];
            if (extPrim.materialIndex >= 0 && (size_t)extPrim.materialIndex < materials.size()) {
                prim.material = &materials[extPrim.materialIndex];
            }
        }

        meshes[m].primitives_count = meshPrimitives[m].size();
        meshes[m].primitives = meshPrimitives[m].data();
    }

    // Polaczenie Animacji z Samplerami i Kanalami
    for (size_t a = 0; a < animCount; ++a) {
        size_t chanCount = animChannelMetas[a].size();
        animSamplers[a].resize(chanCount);
        animChannels[a].resize(chanCount);

        for (size_t c = 0; c < chanCount; ++c) {
            const auto& meta = animChannelMetas[a][c];

            cgltf_animation_sampler& sampler = animSamplers[a][c];
            sampler.input = &accessors[meta.timeAccIdx];
            sampler.output = &accessors[meta.dataAccIdx];
            sampler.interpolation = cgltf_interpolation_type_linear;

            cgltf_animation_channel& channel = animChannels[a][c];
            channel.sampler = &animSamplers[a][c];
            channel.target_node = &nodes[meta.nodeIdx];
            channel.target_path = meta.pathType;
        }

        animations[a].samplers_count = animSamplers[a].size();
        animations[a].samplers = animSamplers[a].empty() ? nullptr : animSamplers[a].data();
        animations[a].channels_count = animChannels[a].size();
        animations[a].channels = animChannels[a].empty() ? nullptr : animChannels[a].data();

        if (!extractedAnims[a].events.empty()) {
            std::string evJson = BuildExtrasJson(extractedAnims[a].events);
            animations[a].extras.data = _strdup(evJson.c_str());
        }
    }

    // Scena
    std::vector<cgltf_scene> scenes(1);
    scenes[0].name = _strdup("DefaultScene");
    scenes[0].nodes_count = sceneNodes.size();
    scenes[0].nodes = sceneNodes.empty() ? nullptr : sceneNodes.data();

    // Glowna struktura cgltf_data
    cgltf_data data = {};
    data.asset.version = (char*)"2.0";
    data.asset.generator = (char*)"Granny to glTF 2.0 Converter";

    std::string extrasJson = BuildExtrasJson(extractor.GetEvents());
    if (!extrasJson.empty()) {
        data.asset.extras.data = _strdup(extrasJson.c_str());
    }

    data.buffers_count = buffers.size();
    data.buffers = buffers.data();

    data.buffer_views_count = bufferViews.size();
    data.buffer_views = bufferViews.empty() ? nullptr : bufferViews.data();

    data.accessors_count = accessors.size();
    data.accessors = accessors.empty() ? nullptr : accessors.data();

    data.materials_count = materials.size();
    data.materials = materials.empty() ? nullptr : materials.data();

    data.meshes_count = meshes.size();
    data.meshes = meshes.empty() ? nullptr : meshes.data();

    data.skins_count = skins.size();
    data.skins = skins.empty() ? nullptr : skins.data();

    data.nodes_count = nodes.size();
    data.nodes = nodes.empty() ? nullptr : nodes.data();

    data.animations_count = animations.size();
    data.animations = animations.empty() ? nullptr : animations.data();

    data.scenes_count = scenes.size();
    data.scenes = scenes.data();
    data.scene = &scenes[0];

    data.bin = binBuffer.data();
    data.bin_size = binBuffer.size();

    cgltf_options options = {};
    options.type = cgltf_file_type_glb;

    cgltf_result result = cgltf_write_file(&options, outputPath.c_str(), &data);

    if (data.asset.extras.data) {
        free(data.asset.extras.data);
    }
    for (size_t a = 0; a < animCount; ++a) {
        if (animations[a].extras.data) {
            free(animations[a].extras.data);
        }
    }
    for (size_t m = 0; m < materials.size(); ++m) {
        if (materials[m].extras.data) {
            free(materials[m].extras.data);
        }
    }

    if (result != cgltf_result_success) {
        std::cerr << "Blad podczas zapisu pliku GLB: " << result << std::endl;
        return false;
    }

    return true;
}

std::string GlbWriter::BuildExtrasJson(const std::vector<Metin2Event>& events) {
    if (events.empty()) return "";

    std::ostringstream ss;
    ss << "{\"metin2_events\":[";
    for (size_t i = 0; i < events.size(); ++i) {
        std::string safeType;
        for (char c : events[i].type) {
            if (c == '\"') safeType += "\\\"";
            else if (c == '\\') safeType += "\\\\";
            else if ((unsigned char)c >= 32 && (unsigned char)c <= 126) safeType += c;
            else safeType += ' ';
        }
        ss << "{\"time\":" << events[i].time << ",\"type\":\"" << safeType << "\"}";
        if (i < events.size() - 1) {
            ss << ",";
        }
    }
    ss << "]}";
    return ss.str();
}

std::string GlbWriter::BuildMaterialExtrasJson(const ExtractedMaterial& mat) {
    if (mat.diffuseTexture.empty() && mat.opacityTexture.empty()) return "";

    std::ostringstream ss;
    ss << "{";
    bool first = true;
    if (!mat.diffuseTexture.empty()) {
        std::string safeDiff;
        for (char c : mat.diffuseTexture) {
            if (c == '\"') safeDiff += "\\\"";
            else if (c == '\\') safeDiff += "/";
            else safeDiff += c;
        }
        ss << "\"diffuse_texture\":\"" << safeDiff << "\"";
        first = false;
    }
    if (!mat.opacityTexture.empty()) {
        if (!first) ss << ",";
        std::string safeOpac;
        for (char c : mat.opacityTexture) {
            if (c == '\"') safeOpac += "\\\"";
            else if (c == '\\') safeOpac += "/";
            else safeOpac += c;
        }
        ss << "\"opacity_texture\":\"" << safeOpac << "\"";
    }
    ss << "}";
    return ss.str();
}

