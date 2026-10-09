#pragma once

#include "GltfModel.h"
#include "SkeletalMath.h"
#include <vector>
#include <string>

namespace EterModelLib
{

class CGltfModelInstance
{
public:
    CGltfModelInstance();
    explicit CGltfModelInstance(const CGltfModel* pModel);
    virtual ~CGltfModelInstance();

    // Przypisanie i odczyt modelu zrodlowego
    void SetModel(const CGltfModel* pModel);
    const CGltfModel* GetModel() const { return m_pModel; }

    // Wspoldzielony szkielet (Hair Link / Linked Skeleton)
    void LinkSkeleton(const CGltfModelInstance* pParentInstance);
    const CGltfModelInstance* GetLinkedSkeleton() const { return m_pLinkedModelInstance; }
    void SetLinkedModelInstance(const CGltfModelInstance* pParentInstance) { LinkSkeleton(pParentInstance); }
    const CGltfModelInstance* GetLinkedModelInstance() const { return m_pLinkedModelInstance; }

    // Zarzadzanie aktywna animacja
    bool SetAnimation(int animIndex, bool loop = true);
    bool SetAnimation(const std::string& animName, bool loop = true);
    bool PlayMotion(int animIndex, bool loop = true) { return SetAnimation(animIndex, loop); }
    bool PlayMotion(const std::string& animName, bool loop = true) { return SetAnimation(animName, loop); }
    int GetCurrentAnimationIndex() const { return m_currentAnimIndex; }
    const GltfMotionData* GetCurrentAnimation() const;

    // Sterowanie czasem animacji
    float GetCurrentTime() const { return m_currentTime; }
    void SetCurrentTime(float time);
    float GetPrevTime() const { return m_prevTime; }
    float GetDuration() const { return m_duration; }
    bool IsLoop() const { return m_isLoop; }
    void SetLoop(bool loop) { m_isLoop = loop; }
    bool IsFinished() const { return m_isFinished; }
    float GetPlaySpeed() const { return m_playSpeed; }
    void SetPlaySpeed(float speed) { m_playSpeed = speed; }

    // Glowna metoda aktualizacji klatek animacji
    // (LERP pozycji/skali, SLERP kwaternionow ze SkeletalMath, EvaluateHierarchy, ComputeSkinningMatrices)
    void Update(float deltaTime);

    // CPU Skinning wierzcholkow na podstawie macierzy skinningu i wag kosci
    void DeformVertices();
    const std::vector<GltfVertex>& GetDeformedVertices() const { return m_deformedVertices; }

    // Wykrywanie eventow walki w oknie czasowym
    void CheckEvents(float prevTime, float currTime, std::vector<GltfEvent>& outEvents) const;
    void CheckEvents(std::vector<GltfEvent>& outEvents) const;

    // Macierze transformacji kosci
    const std::vector<Matrix4x4>& GetLocalTransforms() const { return m_localTransforms; }
    const std::vector<Matrix4x4>& GetWorldTransforms() const { return m_worldTransforms; }
    const std::vector<Matrix4x4>& GetSkinningMatrices() const { return m_skinningMatrices; }

    // Odczyt macierzy transformacji kosci
    const Matrix4x4* GetBoneMatrixPointer(int boneIndex) const;
    const Matrix4x4* GetBoneMatrixPointer(const char* szBoneName) const;

    // Odczyt macierzy swiatowej kosci (np. do doczepiania broni, efektow czasteczkowych)
    bool GetBoneWorldTransform(size_t boneIndex, Matrix4x4& outTransform) const;
    bool GetBoneWorldTransform(const std::string& boneName, Matrix4x4& outTransform) const;


    // Gniazda kosci (Bone Attachment / Sockets)
    void SetParentModelInstance(const CGltfModelInstance* pParentInstance, const char* szBoneName);
    void SetParentModelInstance(const CGltfModelInstance* pParentInstance, int boneIndex);
    void SetParentBoneMatrix(const Matrix4x4* pParentMatrix);
    const Matrix4x4* GetParentBoneMatrix() const;
    const CGltfModelInstance* GetParentModelInstance() const { return m_pParentInstance; }
    int GetParentBoneIndex() const { return m_parentBoneIndex; }
    const std::string& GetParentBoneName() const { return m_parentBoneName; }

private:
    void UpdateTransforms(float time);

private:
    const CGltfModel* m_pModel;
    const CGltfModelInstance* m_pLinkedModelInstance;

    // Gniazdo kosci (Bone Attachment / Socket)
    const CGltfModelInstance* m_pParentInstance;
    std::string m_parentBoneName;
    int m_parentBoneIndex;
    Matrix4x4 m_parentBoneMatrix;
    bool m_bHasParentBoneMatrix;

    int m_currentAnimIndex;
    float m_currentTime;
    float m_prevTime;
    float m_duration;
    bool m_isLoop;
    bool m_isFinished;
    float m_playSpeed;

    std::vector<Matrix4x4> m_localTransforms;
    std::vector<Matrix4x4> m_worldTransforms;
    std::vector<Matrix4x4> m_skinningMatrices;

    std::vector<GltfVertex> m_deformedVertices;
};

} // namespace EterModelLib

using CGltfModelInstance = EterModelLib::CGltfModelInstance;
