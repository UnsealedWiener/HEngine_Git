#include"RasterizeRootSig.hlsl"
#include"lighting.hlsl"
struct PixelIn
{
    float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3x3 TangentBasis : TBASIS;
    float2 TexC    : TEXCOORD;
    // nointerpolation is used so the index is not interpolated 
    // across the triangle.
    nointerpolation uint instID : INSTID;
};

struct PS_OUTPUT
{
    float4 albedo : SV_Target0;
    float4 metallicRoughnessAo : SV_Target1;
    float4 normal : SV_Target2;
    float4 emissive : SV_Target3;
};

PS_OUTPUT PS(PixelIn pin, uint coverage : SV_Coverage)
{
    InstanceData instData = gInstanceData[pin.instID];
    uint matIndex = instData.MatIndex0;

    float3 Albedo   = baseColor_colorChip.Sample(gsamLinearWrap, pin.TexC);
    float Roughness = roughness_colorChip.Sample(gsamLinearWrap, pin.TexC).x;
    float Metallic  = metallic_colorChip.Sample(gsamLinearWrap, pin.TexC).x;
    float emissive = emissive_colorChip.Sample(gsamLinearWrap, pin.TexC).x;
    float Ao        = 1.f;

    //Outgoing light direction(월드 스페이스 픽셀에서 눈으로의 벡터)
    //float3 Lo = normalize(gEyePosW - pin.PosW);

    //Normal(픽셀에서의 노말)
    float3 N = normalize(pin.TangentBasis[2]);


    PS_OUTPUT psOutput;

    psOutput.albedo = float4(UnGammaCorrection(Albedo), 1);
    //psOutput.albedo = float4(1,0,0, 1);

    psOutput.metallicRoughnessAo = float4(Metallic, Roughness, Ao, 1);
    psOutput.normal = float4(N, 1);
    psOutput.emissive = float4(emissive * psOutput.albedo.xyz, 1);
    //psOutput.edge = 30;

    return psOutput;
  
}


