#include "SceneVariables.hlsl"

#pragma pack_matrix(column_major)

struct ConstData
{
    float4x4        world;
    float4x4        worldInvTrans;
    float4x4        texTransform;
};

ConstantBuffer<sceneVariables> scene : register(b0);
ConstantBuffer<ConstData> constData : register(b1);

Texture2D Texture :    register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);


