#define BUILDING_GRANNY_STATIC 1
#define BUILDING_GRANNY 1
#include "StdAfx.h"
#include <cstring>

// Definicja zmiennych globalnych Granny
granny_data_type_definition* GrannyPNT332VertexType = nullptr;

extern "C" {

granny_file* GrannyReadEntireFileFromMemory(granny_int32x MemorySize, void const* Memory)
{
    return nullptr;
}

granny_file_info* GrannyGetFileInfo(granny_file* File)
{
    return nullptr;
}

void GrannyFreeFile(granny_file* File)
{
}

void GrannyFreeFileSection(granny_file* File, granny_int32x SectionIndex)
{
}

granny_control* GrannyPlayControlledAnimation(granny_real32 StartTime, granny_animation const* Animation, granny_model_instance* ModelInstance)
{
    return nullptr;
}

bool GrannyFreeControlIfComplete(granny_control* Control)
{
    return true;
}

void GrannyFreeControlOnceUnused(granny_control* Control)
{
}

void GrannySetControlSpeed(granny_control* Control, granny_real32 Speed)
{
}

granny_real32 GrannyGetControlSpeed(granny_control const* Control)
{
    return 1.0f;
}

void GrannySetControlLoopCount(granny_control* Control, granny_int32x LoopCount)
{
}

granny_int32x GrannyGetControlLoopCount(granny_control const* Control)
{
    return 0;
}

void GrannySetControlEaseIn(granny_control* Control, bool EaseIn)
{
}

void GrannySetControlEaseInCurve(granny_control* Control, granny_real32 Duration, granny_real32 InY0, granny_real32 InY1, granny_real32 OutY0, granny_real32 OutY1, granny_real32 EaseCurve)
{
}

void GrannySetControlEaseOut(granny_control* Control, bool EaseOut)
{
}

void GrannySetControlEaseOutCurve(granny_control* Control, granny_real32 Duration, granny_real32 InY0, granny_real32 InY1, granny_real32 OutY0, granny_real32 OutY1, granny_real32 EaseCurve)
{
}

void GrannyCompleteControlAt(granny_control* Control, granny_real32 Duration)
{
}

granny_real32 GrannyGetControlLocalDuration(granny_control const* Control)
{
    return 0.0f;
}

granny_real32 GrannyGetControlRawLocalClock(granny_control* Control)
{
    return 0.0f;
}

void GrannySetControlRawLocalClock(granny_control* Control, granny_real32 Clock)
{
}

bool GrannyControlIsComplete(granny_control const* Control)
{
    return true;
}

granny_real32* GrannyGetWorldPose4x4(granny_world_pose const* WorldPose, granny_int32x BoneIndex)
{
    static float s_ident[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    return s_ident;
}

granny_real32* GrannyGetWorldPoseComposite4x4(granny_world_pose const* WorldPose, granny_int32x BoneIndex)
{
    static float s_ident[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    return s_ident;
}

granny_matrix_4x4* GrannyGetWorldPoseComposite4x4Array(granny_world_pose const* WorldPose)
{
    static granny_matrix_4x4 s_identArray[256];
    return s_identArray;
}

void GrannyFreeCompletedModelControls(granny_model_instance const* ModelInstance)
{
}

void GrannySetModelClock(granny_model_instance const* ModelInstance, granny_real32 NewClock)
{
}

void GrannyUpdateModelMatrix(granny_model_instance const* ModelInstance, granny_real32 SecondsElapsed, granny_real32 const* ModelMatrix4x4, granny_real32* DestMatrix4x4, bool Inverse)
{
    if (ModelMatrix4x4 && DestMatrix4x4 && ModelMatrix4x4 != DestMatrix4x4)
        memcpy(DestMatrix4x4, ModelMatrix4x4, sizeof(float) * 16);
}

granny_skeleton* GrannyGetSourceSkeleton(granny_model_instance const* Model)
{
    return nullptr;
}

void GrannySampleModelAnimations(granny_model_instance const* ModelInstance, granny_int32x FirstBone, granny_int32x BoneCount, granny_local_pose* LocalPose)
{
}

void GrannySampleModelAnimationsAccelerated(granny_model_instance const* ModelInstance, granny_int32x BoneCount, granny_real32 const* Offset4x4, granny_local_pose* LocalPose, granny_world_pose* WorldPose)
{
}

void GrannyBuildWorldPose(granny_skeleton const* Skeleton, granny_int32x FirstBone, granny_int32x BoneCount, granny_local_pose const* LocalPose, granny_real32 const* Offset4x4, granny_world_pose* WorldPose)
{
}

granny_model_instance* GrannyInstantiateModel(granny_model const* Model)
{
    return nullptr;
}

void GrannyFreeModelInstance(granny_model_instance* ModelInstance)
{
}

granny_world_pose* GrannyNewWorldPose(granny_int32x BoneCount)
{
    return nullptr;
}

void GrannyFreeWorldPose(granny_world_pose* WorldPose)
{
}

granny_local_pose* GrannyNewLocalPose(granny_int32x BoneCount)
{
    return nullptr;
}

void GrannyFreeLocalPose(granny_local_pose* LocalPose)
{
}

granny_mesh_binding* GrannyNewMeshBinding(granny_mesh const* Mesh, granny_skeleton const* FromSkeleton, granny_skeleton const* ToSkeleton)
{
    return nullptr;
}

void GrannyFreeMeshBinding(granny_mesh_binding* Binding)
{
}

granny_int32x const* GrannyGetMeshBindingToBoneIndices(granny_mesh_binding const* Binding)
{
    return nullptr;
}

granny_int32x GrannyGetMeshIndexCount(granny_mesh const* Mesh)
{
    return 0;
}

granny_int32x GrannyGetMeshVertexCount(granny_mesh const* Mesh)
{
    return 0;
}

granny_data_type_definition* GrannyGetMeshVertexType(granny_mesh const* Mesh)
{
    return nullptr;
}

void* GrannyGetMeshVertices(granny_mesh const* Mesh)
{
    return nullptr;
}

void* GrannyGetMeshIndices(granny_mesh const* Mesh)
{
    return nullptr;
}

void GrannyCopyMeshVertices(granny_mesh const* Mesh, granny_data_type_definition const* VertexType, void* DestVertices)
{
}

void GrannyCopyMeshIndices(granny_mesh const* Mesh, granny_int32x BytesPerIndex, void* DestIndices)
{
}

granny_int32x GrannyGetMeshTriangleGroupCount(granny_mesh const* Mesh)
{
    return 0;
}

granny_tri_material_group* GrannyGetMeshTriangleGroups(granny_mesh const* Mesh)
{
    return nullptr;
}

bool GrannyMeshIsRigid(granny_mesh const* Mesh)
{
    return true;
}

granny_mesh_deformer* GrannyNewMeshDeformer(granny_data_type_definition const* InputVertexLayout, granny_data_type_definition const* OutputVertexLayout, granny_deformation_type DeformationType, granny_deformer_tail_flags TailFlag)
{
    return nullptr;
}

void GrannyFreeMeshDeformer(granny_mesh_deformer* Deformer)
{
}

void GrannyDeformVertices(granny_mesh_deformer const* Deformer, granny_int32x const* TransformIndices, granny_real32 const* Transforms, granny_int32x VertexCount, void const* SourceVertices, void* DestVertices)
{
}

granny_texture* GrannyGetMaterialTextureByType(granny_material const* Material, granny_material_texture_type Type)
{
    return nullptr;
}

void GrannyFreeControl(granny_control* Control)
{
}

bool GrannyFindMatchingMember(granny_data_type_definition const* SourceType,
                              void const* SourceObject,
                              char const* DestMemberName,
                              granny_variant* Result)
{
    return false;
}

void GrannyConvertSingleObject(granny_data_type_definition const* SourceType,
                               void const* SourceObject,
                               granny_data_type_definition const* DestType,
                               void* DestObject,
                               granny_conversion_handler* OverrideHandler)
{
}

bool GrannyFindBoneByName(granny_skeleton const* Skeleton,
                          char const* BoneName,
                          granny_int32x* BoneIndex)
{
    if (BoneIndex) *BoneIndex = -1;
    return false;
}

} // extern "C"
