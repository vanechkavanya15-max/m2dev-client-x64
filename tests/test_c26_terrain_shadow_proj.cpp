#define TEST_MOCK_D3D9

#include <cmath>
#include <cstdint>
#include <cassert>

typedef struct _D3DVECTOR {
    float x, y, z;
} D3DVECTOR;

typedef struct _D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
        };
        float m[4][4];
    };
} D3DMATRIX;

typedef D3DMATRIX D3DXMATRIX;

typedef struct _D3DXVECTOR3 {
    float x;
    float y;
    float z;
    _D3DXVECTOR3() {}
    _D3DXVECTOR3(float _x, float _y, float _z) {
        x = _x;
        y = _y;
        z = _z;
    }
} D3DXVECTOR3;

inline D3DXVECTOR3 operator-(const D3DXVECTOR3& a, const D3DXVECTOR3& b) {
    return D3DXVECTOR3(a.x - b.x, a.y - b.y, a.z - b.z);
}
inline D3DXVECTOR3 operator*(const D3DXVECTOR3& a, float b) {
    return D3DXVECTOR3(a.x * b, a.y * b, a.z * b);
}

inline D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV) {
    float len = std::sqrt(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
    if (len > 0.0f) {
        pOut->x = pV->x / len;
        pOut->y = pV->y / len;
        pOut->z = pV->z / len;
    } else {
        pOut->x = 0.0f; pOut->y = 0.0f; pOut->z = 0.0f;
    }
    return pOut;
}

inline float D3DXVec3Dot(const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2) {
    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

inline D3DXVECTOR3* D3DXVec3Cross(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2) {
    D3DXVECTOR3 v;
    v.x = pV1->y * pV2->z - pV1->z * pV2->y;
    v.y = pV1->z * pV2->x - pV1->x * pV2->z;
    v.z = pV1->x * pV2->y - pV1->y * pV2->x;
    *pOut = v;
    return pOut;
}

inline D3DMATRIX* D3DXMatrixLookAtLH(D3DMATRIX* pOut, const D3DXVECTOR3* pEye, const D3DXVECTOR3* pAt, const D3DXVECTOR3* pUp) {
    D3DXVECTOR3 zaxis;
    D3DXVECTOR3 dir(pAt->x - pEye->x, pAt->y - pEye->y, pAt->z - pEye->z);
    D3DXVec3Normalize(&zaxis, &dir);
    
    D3DXVECTOR3 xaxis;
    D3DXVec3Cross(&xaxis, pUp, &zaxis);
    D3DXVec3Normalize(&xaxis, &xaxis);
    
    D3DXVECTOR3 yaxis;
    D3DXVec3Cross(&yaxis, &zaxis, &xaxis);

    pOut->_11 = xaxis.x; pOut->_12 = yaxis.x; pOut->_13 = zaxis.x; pOut->_14 = 0.0f;
    pOut->_21 = xaxis.y; pOut->_22 = yaxis.y; pOut->_23 = zaxis.y; pOut->_24 = 0.0f;
    pOut->_31 = xaxis.z; pOut->_32 = yaxis.z; pOut->_33 = zaxis.z; pOut->_34 = 0.0f;
    pOut->_41 = -D3DXVec3Dot(&xaxis, pEye);
    pOut->_42 = -D3DXVec3Dot(&yaxis, pEye);
    pOut->_43 = -D3DXVec3Dot(&zaxis, pEye);
    pOut->_44 = 1.0f;
    return pOut;
}

inline D3DMATRIX* D3DXMatrixOrthoLH(D3DMATRIX* pOut, float w, float h, float zn, float zf) {
    pOut->_11 = 2.0f / w; pOut->_12 = 0.0f;     pOut->_13 = 0.0f;           pOut->_14 = 0.0f;
    pOut->_21 = 0.0f;     pOut->_22 = 2.0f / h; pOut->_23 = 0.0f;           pOut->_24 = 0.0f;
    pOut->_31 = 0.0f;     pOut->_32 = 0.0f;     pOut->_33 = 1.0f/(zf-zn);   pOut->_34 = 0.0f;
    pOut->_41 = 0.0f;     pOut->_42 = 0.0f;     pOut->_43 = -zn/(zf-zn);    pOut->_44 = 1.0f;
    return pOut;
}

inline D3DMATRIX* D3DXMatrixMultiply(D3DMATRIX* pOut, const D3DMATRIX* pM1, const D3DMATRIX* pM2) {
    D3DMATRIX m;
    for (int i=0; i<4; ++i) {
        for (int j=0; j<4; ++j) {
            m.m[i][j] = pM1->m[i][0]*pM2->m[0][j] +
                        pM1->m[i][1]*pM2->m[1][j] +
                        pM1->m[i][2]*pM2->m[2][j] +
                        pM1->m[i][3]*pM2->m[3][j];
        }
    }
    *pOut = m;
    return pOut;
}

inline void D3DXMatrixIdentity(D3DMATRIX* pOut) {
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            pOut->m[i][j] = (i==j) ? 1.0f : 0.0f;
}

#include "../src/EterLib/Render/TerrainShadowMapProjector.cpp"

bool is_close(float a, float b) { return std::abs(a - b) < 0.001f; }

int main()
{
    EterLib::Render::TerrainShadowMapProjector projector;

    D3DMATRIX m = projector.GetLightViewProjMatrix();
    assert(is_close(m.m[0][0], 1.0f));

    D3DVECTOR sunDir = { 1.0f, 0.0f, -1.0f };
    D3DVECTOR cameraPos = { 0.0f, 0.0f, 0.0f };

    projector.SetupSunMatrix(sunDir, cameraPos);

    D3DMATRIX m2 = projector.GetLightViewProjMatrix();
    // Validate we actually generated some valid matrix
    assert(m2.m[3][3] == 1.0f);

    // IsPatchInShadow check
    // Given the orthographic setup w/h = 10000, 
    // center should definitely be in shadow
    bool inShadow = projector.IsPatchInShadow(0.0f, 0.0f, 100.0f);
    assert(inShadow == true);

    // Patch way out
    bool outShadow = projector.IsPatchInShadow(999999.0f, 999999.0f, 100.0f);
    assert(outShadow == false);

    return 0;
}

