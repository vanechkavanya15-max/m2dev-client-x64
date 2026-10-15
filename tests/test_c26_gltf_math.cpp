#include <cassert>
#include <iostream>
#include "../src/EterModelLib/SkeletalMath.h"

using namespace EterModelLib;

bool ApproxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}
bool ApproxEqualVec(const Vector3& a, const Vector3& b, float epsilon = 0.001f) {
    return ApproxEqual(a.x, b.x, epsilon) && ApproxEqual(a.y, b.y, epsilon) && ApproxEqual(a.z, b.z, epsilon);
}
bool ApproxEqualMat(const Matrix4x4& a, const Matrix4x4& b, float epsilon = 0.001f) {
    for(int i=0; i<4; ++i)
        for(int j=0; j<4; ++j)
            if(!ApproxEqual(a.m[i][j], b.m[i][j], epsilon)) return false;
    return true;
}
bool ApproxEqualQuat(const Quaternion& a, const Quaternion& b, float epsilon = 0.001f) {
    return ApproxEqual(a.x, b.x, epsilon) && ApproxEqual(a.y, b.y, epsilon) && ApproxEqual(a.z, b.z, epsilon) && ApproxEqual(a.w, b.w, epsilon);
}

void TestSlerp() {
    Quaternion q1 = Quaternion::Identity();
    Quaternion q2(0, 0, 1, 0); // 180 degrees around z
    Quaternion res = Quaternion::Slerp(q1, q2, 0.5f);
    assert(ApproxEqual(res.z, 0.7071f) && ApproxEqual(res.w, 0.7071f));
    std::cout << "TestSlerp passed" << std::endl;
}

void TestLerp() {
    Vector3 v1(0, 0, 0);
    Vector3 v2(10, 0, 0);
    Vector3 res = Vector3::Lerp(v1, v2, 0.5f);
    assert(ApproxEqualVec(res, Vector3(5, 0, 0)));
    std::cout << "TestLerp passed" << std::endl;
}

void TestTRS() {
    Vector3 pos(1, 2, 3);
    Quaternion rot = Quaternion::Identity();
    Vector3 scale(2, 2, 2);
    Matrix4x4 mat = Matrix4x4::TRS(pos, rot, scale);
    assert(ApproxEqual(mat.m[0][0], 2.0f));
    assert(ApproxEqual(mat.m[3][0], 1.0f));
    std::cout << "TestTRS passed" << std::endl;
}

void TestHierarchy() {
    std::vector<int32_t> parents = {-1, 0, 1};
    std::vector<Matrix4x4> local(3, Matrix4x4::Identity());
    local[1].m[3][0] = 10.0f; // child at x=10
    local[2].m[3][0] = 5.0f;  // grandchild at local x=5

    std::vector<Matrix4x4> world;
    EvaluateHierarchy(parents, local, world);

    assert(ApproxEqual(world[0].m[3][0], 0.0f));
    assert(ApproxEqual(world[1].m[3][0], 10.0f));
    assert(ApproxEqual(world[2].m[3][0], 15.0f));
    std::cout << "TestHierarchy passed" << std::endl;
}

void TestSkinning() {
    std::vector<Matrix4x4> world(1, Matrix4x4::Identity());
    world[0].m[3][0] = 5.0f;
    
    std::vector<Matrix4x4> invBind(1, Matrix4x4::Identity());
    invBind[0].m[3][0] = -5.0f;
    
    std::vector<Matrix4x4> skin;
    ComputeSkinningMatrices(world, invBind, skin);
    
    assert(ApproxEqual(skin[0].m[3][0], 0.0f));
    std::cout << "TestSkinning passed" << std::endl;
}

int main() {
    TestSlerp();
    TestLerp();
    TestTRS();
    TestHierarchy();
    TestSkinning();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
