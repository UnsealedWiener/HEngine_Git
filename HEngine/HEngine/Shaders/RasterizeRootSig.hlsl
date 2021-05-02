#include "SceneVariables.hlsl"

#pragma pack_matrix(column_major)

struct InstanceData
{
    float4x4        World;
    float4x4        TexTransform;
    unsigned int    MatIndex0;
    unsigned int    MatIndex1;
    float           LerpValue;
    unsigned int    padding;                //structuredBuffer는 꼭 16바이트 정렬일 필요는 없지만 
                                            //정렬일 때 읽는 속도가 빨라진다.
};

struct PerPsoData
{
    uint instanceNumOffset;
    uint padding[3];
};

// An array of textures, which is only supported in shader model 5.1+.  Unlike Texture2DArray, the textures
// in this array can be different sizes and formats, making it more flexible than texture arrays.
//MatIndex + 0 = albedo
//MatIndex + 1 = roughness
//MatIndex + 2 = metallic
//MatIndex + 3 = ao
//MatIndex + 4 = normal
//MatIndex + 5 = height

Texture2D Textures[300] :    register(t0, space0);
TextureCube Skybox      : register(t300, space0);

// Put in space1, so the texture array does not overlap with these resources.  
// The texture array will occupy registers t0, t1, ..., t6 in space0. 
StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<float4x4>     gInstanceBoneTM : register(t1, space1);
StructuredBuffer<PerPsoData>   gPsoData : register(t2, space1);
//aStructuredBuffer<Light>        gLights : register(t3, space1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

cbuffer CBPerModel : register(b0)
{
    float4x4 gAxisChange;
    uint gBoneCount;
    uint padding[3];
};

ConstantBuffer<sceneVariables> scene : register(b1);
