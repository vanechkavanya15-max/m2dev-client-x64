#include "GrannyExtractor.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <set>

static std::string SanitizeName(const std::string& input, const std::string& fallback) {
    if (input.empty()) return fallback;
    std::string s = input;
    size_t lastSlash = s.find_last_of("\\/");
    if (lastSlash != std::string::npos && lastSlash + 1 < s.size()) {
        s = s.substr(lastSlash + 1);
    }
    if (s.size() > 4 && s.substr(s.size() - 4) == ".gr2") {
        s = s.substr(0, s.size() - 4);
    }
    std::string clean;
    for (char c : s) {
        if ((unsigned char)c >= 32 && (unsigned char)c <= 126 && c != '\"' && c != '\\') {
            clean += c;
        } else {
            clean += '_';
        }
    }
    if (clean.empty() || clean.find_first_not_of('_') == std::string::npos) {
        return fallback;
    }
    return clean;
}

GrannyExtractor::GrannyExtractor() : m_file(nullptr), m_fileInfo(nullptr) {
}

GrannyExtractor::~GrannyExtractor() {
    Free();
}

bool GrannyExtractor::Load(const std::string& path) {
    Free();

    m_file = GrannyReadEntireFile(path.c_str());
    if (!m_file) {
        std::cerr << "Nie mozna wczytac pliku: " << path << std::endl;
        return false;
    }

    m_fileInfo = GrannyGetFileInfo(m_file);
    if (!m_fileInfo) {
        std::cerr << "Nie mozna pobrac informacji o pliku." << std::endl;
        Free();
        return false;
    }

    ExtractMaterials();
    ExtractSkeletons();
    ExtractMeshes();
    ExtractAnimations();
    ExtractEvents();

    return true;
}

void GrannyExtractor::Free() {
    if (m_file) {
        GrannyFreeFile(m_file);
        m_file = nullptr;
    }
    m_fileInfo = nullptr;
    m_events.clear();
    m_skeletons.clear();
    m_meshes.clear();
    m_animations.clear();
    m_materials.clear();
}

void GrannyExtractor::ExtractMaterials() {
    m_materials.clear();
    if (!m_fileInfo || m_fileInfo->MaterialCount <= 0) return;

    for (int i = 0; i < m_fileInfo->MaterialCount; ++i) {
        granny_material* mat = m_fileInfo->Materials[i];
        if (!mat) continue;

        ExtractedMaterial extMat;
        std::string rawName = mat->Name ? mat->Name : "";
        extMat.name = SanitizeName(rawName, "Material_" + std::to_string(i));

        granny_texture* diffTex = GrannyGetMaterialTextureByType(mat, GrannyDiffuseColorTexture);
        if (diffTex && diffTex->FromFileName) {
            extMat.diffuseTexture = diffTex->FromFileName;
        }

        granny_texture* opacTex = GrannyGetMaterialTextureByType(mat, GrannyOpacityTexture);
        if (opacTex && opacTex->FromFileName) {
            extMat.opacityTexture = opacTex->FromFileName;
        }

        if (mat->MapCount > 1 && !_strnicmp(rawName.c_str(), "Blend", 5)) {
            if (mat->Maps[0].Material) {
                granny_texture* m0Diff = GrannyGetMaterialTextureByType(mat->Maps[0].Material, GrannyDiffuseColorTexture);
                if (m0Diff && m0Diff->FromFileName) extMat.diffuseTexture = m0Diff->FromFileName;
            }
            if (mat->Maps[1].Material) {
                granny_texture* m1Diff = GrannyGetMaterialTextureByType(mat->Maps[1].Material, GrannyDiffuseColorTexture);
                if (m1Diff && m1Diff->FromFileName) extMat.opacityTexture = m1Diff->FromFileName;
            }
        }

        m_materials.push_back(std::move(extMat));
    }
}


void GrannyExtractor::ExtractSkeletons() {
    if (!m_fileInfo) return;

    std::vector<granny_skeleton*> sourceSkeletons;
    if (m_fileInfo->SkeletonCount > 0 && m_fileInfo->Skeletons) {
        for (int i = 0; i < m_fileInfo->SkeletonCount; ++i) {
            if (m_fileInfo->Skeletons[i]) {
                sourceSkeletons.push_back(m_fileInfo->Skeletons[i]);
            }
        }
    } else if (m_fileInfo->ModelCount > 0 && m_fileInfo->Models) {
        for (int i = 0; i < m_fileInfo->ModelCount; ++i) {
            if (m_fileInfo->Models[i] && m_fileInfo->Models[i]->Skeleton) {
                sourceSkeletons.push_back(m_fileInfo->Models[i]->Skeleton);
            }
        }
    }

    for (size_t s = 0; s < sourceSkeletons.size(); ++s) {
        granny_skeleton* srcSkel = sourceSkeletons[s];
        if (!srcSkel || srcSkel->BoneCount <= 0) continue;

        ExtractedSkeleton extSkel;
        extSkel.name = SanitizeName(srcSkel->Name ? srcSkel->Name : "", "Skeleton_" + std::to_string(s));
        extSkel.bones.resize(srcSkel->BoneCount);

        for (int b = 0; b < srcSkel->BoneCount; ++b) {
            const granny_bone& gb = srcSkel->Bones[b];
            ExtractedBone& eb = extSkel.bones[b];
            eb.name = SanitizeName(gb.Name ? gb.Name : "", "Bone_" + std::to_string(b));
            eb.parentIndex = gb.ParentIndex;

            eb.localTranslation[0] = gb.LocalTransform.Position[0];
            eb.localTranslation[1] = gb.LocalTransform.Position[1];
            eb.localTranslation[2] = gb.LocalTransform.Position[2];

            eb.localRotation[0] = gb.LocalTransform.Orientation[0];
            eb.localRotation[1] = gb.LocalTransform.Orientation[1];
            eb.localRotation[2] = gb.LocalTransform.Orientation[2];
            eb.localRotation[3] = gb.LocalTransform.Orientation[3];

            eb.localScale[0] = gb.LocalTransform.ScaleShear[0][0];
            eb.localScale[1] = gb.LocalTransform.ScaleShear[1][1];
            eb.localScale[2] = gb.LocalTransform.ScaleShear[2][2];
            if (std::abs(eb.localScale[0]) < 1e-6f) eb.localScale[0] = 1.0f;
            if (std::abs(eb.localScale[1]) < 1e-6f) eb.localScale[1] = 1.0f;
            if (std::abs(eb.localScale[2]) < 1e-6f) eb.localScale[2] = 1.0f;

            // Kopiowanie macierzy InverseWorld4x4 (16 floats)
            memcpy(eb.inverseBindMatrix, gb.InverseWorld4x4, 16 * sizeof(float));

            // Jesli macierz jest calkowicie wyzerowana, ustaw macierz tozsamosciowa (identity)
            bool isZero = true;
            for (int k = 0; k < 16; ++k) {
                if (std::abs(eb.inverseBindMatrix[k]) > 1e-6f) {
                    isZero = false;
                    break;
                }
            }
            if (isZero) {
                memset(eb.inverseBindMatrix, 0, 16 * sizeof(float));
                eb.inverseBindMatrix[0] = 1.0f;
                eb.inverseBindMatrix[5] = 1.0f;
                eb.inverseBindMatrix[10] = 1.0f;
                eb.inverseBindMatrix[15] = 1.0f;
            }
        }

        m_skeletons.push_back(std::move(extSkel));
    }
}

void GrannyExtractor::ExtractMeshes() {
    if (!m_fileInfo) return;

    std::vector<granny_mesh*> candidateMeshes;
    std::set<granny_mesh*> visitedMeshes;

    if (m_fileInfo->MeshCount > 0 && m_fileInfo->Meshes) {
        for (int i = 0; i < m_fileInfo->MeshCount; ++i) {
            granny_mesh* m = m_fileInfo->Meshes[i];
            if (m && visitedMeshes.insert(m).second) {
                candidateMeshes.push_back(m);
            }
        }
    }

    if (m_fileInfo->ModelCount > 0 && m_fileInfo->Models) {
        for (int i = 0; i < m_fileInfo->ModelCount; ++i) {
            granny_model* model = m_fileInfo->Models[i];
            if (!model) continue;
            for (int b = 0; b < model->MeshBindingCount; ++b) {
                granny_mesh* m = model->MeshBindings[b].Mesh;
                if (m && visitedMeshes.insert(m).second) {
                    candidateMeshes.push_back(m);
                }
            }
        }
    }

    granny_skeleton* mainSkeleton = nullptr;
    if (m_fileInfo->SkeletonCount > 0 && m_fileInfo->Skeletons && m_fileInfo->Skeletons[0]) {
        mainSkeleton = m_fileInfo->Skeletons[0];
    } else if (m_fileInfo->ModelCount > 0 && m_fileInfo->Models && m_fileInfo->Models[0] && m_fileInfo->Models[0]->Skeleton) {
        mainSkeleton = m_fileInfo->Models[0]->Skeleton;
    }

    for (size_t mIdx = 0; mIdx < candidateMeshes.size(); ++mIdx) {
        granny_mesh* mesh = candidateMeshes[mIdx];
        if (!mesh) continue;

        int vtxCount = GrannyGetMeshVertexCount(mesh);
        int idxCount = GrannyGetMeshIndexCount(mesh);
        if (vtxCount <= 0) continue;

        ExtractedMesh extMesh;
        extMesh.name = SanitizeName(mesh->Name ? mesh->Name : "", "Mesh_" + std::to_string(mIdx));
        extMesh.isRigid = GrannyMeshIsRigid(mesh);
        extMesh.boneBindingCount = mesh->BoneBindingCount;
        extMesh.vertices.resize(vtxCount);

        granny_mesh_binding* meshBinding = nullptr;
        granny_int32x* boneIndices = nullptr;
        if (mainSkeleton && mesh->BoneBindingCount > 0) {
            meshBinding = GrannyNewMeshBinding(mesh, mainSkeleton, mainSkeleton);
            if (meshBinding) {
                boneIndices = (granny_int32x*)GrannyGetMeshBindingToBoneIndices(meshBinding);
            }
        }

        if (extMesh.isRigid) {
            std::vector<granny_pnt332_vertex> grnVertices(vtxCount);
            GrannyCopyMeshVertices(mesh, GrannyPNT332VertexType, grnVertices.data());

            uint16_t rigidBone = 0;
            if (boneIndices && mesh->BoneBindingCount > 0) {
                rigidBone = (uint16_t)boneIndices[0];
            }

            for (int v = 0; v < vtxCount; ++v) {
                const auto& gv = grnVertices[v];
                ExtractedVertex& ev = extMesh.vertices[v];
                ev.position[0] = gv.Position[0];
                ev.position[1] = gv.Position[1];
                ev.position[2] = gv.Position[2];

                ev.normal[0] = gv.Normal[0];
                ev.normal[1] = gv.Normal[1];
                ev.normal[2] = gv.Normal[2];

                ev.uv[0] = gv.UV[0];
                ev.uv[1] = gv.UV[1];

                ev.joints[0] = rigidBone;
                ev.joints[1] = 0;
                ev.joints[2] = 0;
                ev.joints[3] = 0;

                ev.weights[0] = 1.0f;
                ev.weights[1] = 0.0f;
                ev.weights[2] = 0.0f;
                ev.weights[3] = 0.0f;
            }
        } else {
            std::vector<granny_pwnt3432_vertex> grnVertices(vtxCount);
            GrannyCopyMeshVertices(mesh, GrannyPWNT3432VertexType, grnVertices.data());

            for (int v = 0; v < vtxCount; ++v) {
                const auto& gv = grnVertices[v];
                ExtractedVertex& ev = extMesh.vertices[v];
                ev.position[0] = gv.Position[0];
                ev.position[1] = gv.Position[1];
                ev.position[2] = gv.Position[2];

                ev.normal[0] = gv.Normal[0];
                ev.normal[1] = gv.Normal[1];
                ev.normal[2] = gv.Normal[2];

                ev.uv[0] = gv.UV[0];
                ev.uv[1] = gv.UV[1];

                float totalWeight = (float)(gv.BoneWeights[0] + gv.BoneWeights[1] + gv.BoneWeights[2] + gv.BoneWeights[3]);
                if (totalWeight <= 0.0001f) {
                    totalWeight = 255.0f;
                }

                for (int k = 0; k < 4; ++k) {
                    uint8_t localBone = gv.BoneIndices[k];
                    uint16_t mappedBone = 0;
                    if (boneIndices && localBone < mesh->BoneBindingCount) {
                        mappedBone = (uint16_t)boneIndices[localBone];
                    } else {
                        mappedBone = (uint16_t)localBone;
                    }
                    ev.joints[k] = mappedBone;
                    ev.weights[k] = (float)gv.BoneWeights[k] / totalWeight;
                }
            }
        }

        if (meshBinding) {
            GrannyFreeMeshBinding(meshBinding);
            meshBinding = nullptr;
        }

        // Kopiowanie indeksow
        std::vector<uint32_t> allIndices(idxCount);
        if (idxCount > 0) {
            GrannyCopyMeshIndices(mesh, sizeof(uint32_t), allIndices.data());
        }

        int triGroupCount = GrannyGetMeshTriangleGroupCount(mesh);
        granny_tri_material_group* triGroups = GrannyGetMeshTriangleGroups(mesh);

        if (triGroupCount > 0 && triGroups) {
            for (int g = 0; g < triGroupCount; ++g) {
                ExtractedPrimitive prim;
                prim.materialIndex = triGroups[g].MaterialIndex;
                int idxStart = triGroups[g].TriFirst * 3;
                int count = triGroups[g].TriCount * 3;
                if (idxStart >= 0 && idxStart + count <= (int)allIndices.size() && count > 0) {
                    prim.indices.assign(allIndices.begin() + idxStart, allIndices.begin() + idxStart + count);
                    extMesh.primitives.push_back(std::move(prim));
                }
            }
        }

        if (extMesh.primitives.empty() && !allIndices.empty()) {
            ExtractedPrimitive prim;
            prim.materialIndex = 0;
            prim.indices = std::move(allIndices);
            extMesh.primitives.push_back(std::move(prim));
        }

        m_meshes.push_back(std::move(extMesh));
    }
}

void GrannyExtractor::ExtractAnimations() {
    if (!m_fileInfo) return;

    std::vector<granny_animation*> sourceAnims;
    if (m_fileInfo->AnimationCount > 0 && m_fileInfo->Animations) {
        for (int i = 0; i < m_fileInfo->AnimationCount; ++i) {
            if (m_fileInfo->Animations[i]) {
                sourceAnims.push_back(m_fileInfo->Animations[i]);
            }
        }
    }

    for (size_t a = 0; a < sourceAnims.size(); ++a) {
        granny_animation* anim = sourceAnims[a];
        if (!anim) continue;

        ExtractedAnimation extAnim;
        extAnim.name = SanitizeName(anim->Name ? anim->Name : "", "Animation_" + std::to_string(a));
        extAnim.duration = anim->Duration > 0.0f ? anim->Duration : (1.0f / 30.0f);
        extAnim.timeStep = anim->TimeStep > 0.0001f ? anim->TimeStep : (1.0f / 30.0f);

        int sampleCount = (int)std::ceil(extAnim.duration / extAnim.timeStep) + 1;
        if (sampleCount < 2) {
            sampleCount = 2;
        }

        extAnim.timeStamps.resize(sampleCount);
        for (int s = 0; s < sampleCount; ++s) {
            float t = (sampleCount > 1) ? std::min((float)s * extAnim.timeStep, extAnim.duration) : 0.0f;
            if (s == sampleCount - 1) {
                t = extAnim.duration;
            }
            extAnim.timeStamps[s] = t;
        }

        for (int tgIdx = 0; tgIdx < anim->TrackGroupCount; ++tgIdx) {
            granny_track_group* tg = anim->TrackGroups[tgIdx];
            if (!tg) continue;

            // TextTracks
            for (int ttIdx = 0; ttIdx < tg->TextTrackCount; ++ttIdx) {
                granny_text_track* textTrack = &tg->TextTracks[ttIdx];
                for (int e = 0; e < textTrack->EntryCount; ++e) {
                    Metin2Event ev;
                    ev.time = textTrack->Entries[e].TimeStamp;
                    ev.type = textTrack->Entries[e].Text ? textTrack->Entries[e].Text : "";
                    extAnim.events.push_back(ev);
                }
            }

            // TransformTracks
            for (int trIdx = 0; trIdx < tg->TransformTrackCount; ++trIdx) {
                const granny_transform_track& track = tg->TransformTracks[trIdx];
                ExtractedAnimationChannel chan;
                chan.boneName = SanitizeName(track.Name ? track.Name : "", "Track_" + std::to_string(trIdx));
                chan.translations.resize(sampleCount * 3);
                chan.rotations.resize(sampleCount * 4);
                chan.scales.resize(sampleCount * 3);

                const float defPos[3] = {0.0f, 0.0f, 0.0f};
                const float defRot[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                const float defSS[9] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

                for (int s = 0; s < sampleCount; ++s) {
                    float t = extAnim.timeStamps[s];

                    // Probkowanie pozycji (vec3)
                    float pos[3] = {0.0f, 0.0f, 0.0f};
                    GrannyEvaluateCurveAtT(3, false, false, &track.PositionCurve, false, extAnim.duration, t, pos, defPos);
                    chan.translations[s * 3 + 0] = pos[0];
                    chan.translations[s * 3 + 1] = pos[1];
                    chan.translations[s * 3 + 2] = pos[2];

                    // Probkowanie rotacji (kwaternion)
                    float rot[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                    GrannyEvaluateCurveAtT(4, true, false, &track.OrientationCurve, false, extAnim.duration, t, rot, defRot);
                    float len = std::sqrt(rot[0]*rot[0] + rot[1]*rot[1] + rot[2]*rot[2] + rot[3]*rot[3]);
                    if (len > 1e-6f) {
                        rot[0] /= len; rot[1] /= len; rot[2] /= len; rot[3] /= len;
                    } else {
                        rot[0] = 0.0f; rot[1] = 0.0f; rot[2] = 0.0f; rot[3] = 1.0f;
                    }
                    chan.rotations[s * 4 + 0] = rot[0];
                    chan.rotations[s * 4 + 1] = rot[1];
                    chan.rotations[s * 4 + 2] = rot[2];
                    chan.rotations[s * 4 + 3] = rot[3];

                    // Probkowanie skali (vec3 ze skosnej ScaleShear 3x3)
                    float ss[9] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
                    GrannyEvaluateCurveAtT(9, false, false, &track.ScaleShearCurve, false, extAnim.duration, t, ss, defSS);
                    chan.scales[s * 3 + 0] = ss[0];
                    chan.scales[s * 3 + 1] = ss[4];
                    chan.scales[s * 3 + 2] = ss[8];
                }

                extAnim.channels.push_back(std::move(chan));
            }
        }

        m_animations.push_back(std::move(extAnim));
    }
}

void GrannyExtractor::ExtractEvents() {
    m_events.clear();
    for (const auto& anim : m_animations) {
        for (const auto& ev : anim.events) {
            m_events.push_back(ev);
        }
    }
}
