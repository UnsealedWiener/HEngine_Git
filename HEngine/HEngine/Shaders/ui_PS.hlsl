#include"ui_structure.hlsl"

struct PixelIn
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD;
    nointerpolation uint instID : INSTID;
};



float4 PS(PixelIn pixelIn) : SV_Target
{
    StructureBufferPerUiInstance instData = gInstanceData[pixelIn.instID];

    float4 baseColor = gTextures[instData.textureIndex].Sample(gsamLinearWrap,
       pixelIn.uv);

    baseColor.w = baseColor.w * instData.opacity;

    clip(baseColor.w <= 0.03f ? -1 : 1);

    return baseColor;
}
