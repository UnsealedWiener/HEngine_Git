#pragma pack_matrix(column_major)

struct StructureBufferPerUiInstance
{
    float4x4 worldProjTM;
    uint textureIndex;
    float    opacity;
    float    padding[2];
};

StructuredBuffer<StructureBufferPerUiInstance> gInstanceData : register(t0, space0);
Texture2D gTextures[200] : register(t0, space1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);