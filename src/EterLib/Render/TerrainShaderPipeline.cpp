#include "TerrainShaderPipeline.h"
#include <d3dx9.h>

namespace EterLib::Render {

namespace {

const char* g_terrainShaderHLSL = R"(
float4x4 ViewProj : register(c0);
float4x4 World : register(c4);
float4 LightDir : register(c8); // Vertex Shader light direction (to pass to PS)

struct VS_INPUT {
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct VS_OUTPUT {
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
    float3 NormalWorld : TEXCOORD1; // Pass normal to PS for Phong
};

VS_OUTPUT VS_Main(VS_INPUT input) {
    VS_OUTPUT output;
    float4 worldPos = mul(input.Pos, World);
    output.Pos = mul(worldPos, ViewProj);
    output.Tex = input.Tex;

    // Transform normal to world space and pass to PS for per-pixel lighting
    output.NormalWorld = mul(input.Normal, (float3x3)World);

    return output;
}

sampler2D TexSplat : register(s0);
sampler2D TexLayer0 : register(s1);
sampler2D TexLayer1 : register(s2);
sampler2D TexLayer2 : register(s3);
sampler2D TexLayer3 : register(s4);

float4 PS_LightDir : register(c0); // Pixel Shader light direction

float4 PS_Main(VS_OUTPUT input) : COLOR {
    float4 splatMap = tex2D(TexSplat, input.Tex);
    
    // Sample terrain layers
    float4 c0 = tex2D(TexLayer0, input.Tex * 10.0f);
    float4 c1 = tex2D(TexLayer1, input.Tex * 10.0f);
    float4 c2 = tex2D(TexLayer2, input.Tex * 10.0f);
    float4 c3 = tex2D(TexLayer3, input.Tex * 10.0f);
    
    // Multi-layer terrain blending (splatting up to 4 textures)
    float4 finalColor = c0 * splatMap.r + c1 * splatMap.g + c2 * splatMap.b + c3 * splatMap.a;
    
    // Phong shading: per-pixel lighting calculation
    float3 normal = normalize(input.NormalWorld);
    float nDotL = max(0.0f, dot(normal, -PS_LightDir.xyz));
    float3 lightIntensity = saturate(float3(nDotL, nDotL, nDotL) + 0.2f); // Add ambient
    
    return finalColor * float4(lightIntensity, 1.0f);
}
)";

} // namespace

TerrainShaderPipeline::~TerrainShaderPipeline() {
    if (m_vertexShader) {
        m_vertexShader->Release();
        m_vertexShader = nullptr;
    }
    if (m_pixelShader) {
        m_pixelShader->Release();
        m_pixelShader = nullptr;
    }
}

bool TerrainShaderPipeline::Initialize(LPDIRECT3DDEVICE9 dev) {
    if (!dev) return false;

    ID3DXBuffer* vsCode = nullptr;
    ID3DXBuffer* psCode = nullptr;
    ID3DXBuffer* errorMsgs = nullptr;

    // Compile Vertex Shader
    if (FAILED(D3DXCompileShader(g_terrainShaderHLSL, static_cast<UINT>(std::string_view(g_terrainShaderHLSL).length()), 
                                 nullptr, nullptr, "VS_Main", "vs_3_0", 0, &vsCode, &errorMsgs, nullptr))) {
        if (errorMsgs) errorMsgs->Release();
        return false;
    }

    if (FAILED(dev->CreateVertexShader(static_cast<DWORD*>(vsCode->GetBufferPointer()), &m_vertexShader))) {
        vsCode->Release();
        return false;
    }
    vsCode->Release();

    // Compile Pixel Shader
    if (FAILED(D3DXCompileShader(g_terrainShaderHLSL, static_cast<UINT>(std::string_view(g_terrainShaderHLSL).length()), 
                                 nullptr, nullptr, "PS_Main", "ps_3_0", 0, &psCode, &errorMsgs, nullptr))) {
        if (errorMsgs) errorMsgs->Release();
        return false;
    }

    if (FAILED(dev->CreatePixelShader(static_cast<DWORD*>(psCode->GetBufferPointer()), &m_pixelShader))) {
        psCode->Release();
        return false;
    }
    psCode->Release();

    return true;
}

void TerrainShaderPipeline::Bind(LPDIRECT3DDEVICE9 dev, const D3DMATRIX& viewProj, const D3DMATRIX& world, const D3DVECTOR& lightDir) {
    if (!dev) return;

    if (m_vertexShader) {
        dev->SetVertexShader(m_vertexShader);
    }
    if (m_pixelShader) {
        dev->SetPixelShader(m_pixelShader);
    }

    dev->SetVertexShaderConstantF(0, (const float*)&viewProj, 4);
    dev->SetVertexShaderConstantF(4, (const float*)&world, 4);

    float lightData[4] = { lightDir.x, lightDir.y, lightDir.z, 1.0f };
    dev->SetVertexShaderConstantF(8, lightData, 1);
    dev->SetPixelShaderConstantF(0, lightData, 1);
}

void TerrainShaderPipeline::Unbind(LPDIRECT3DDEVICE9 dev) noexcept {
    if (!dev) return;
    
    dev->SetVertexShader(nullptr);
    dev->SetPixelShader(nullptr);
}

} // namespace EterLib::Render

