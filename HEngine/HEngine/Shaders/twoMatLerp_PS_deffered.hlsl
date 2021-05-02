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
};

PS_OUTPUT PS(PixelIn pin)
{

    InstanceData instData = gInstanceData[pin.instID];

    float LerpValue = instData.LerpValue;
    LerpValue = LerpValue * LerpValue * LerpValue;
    float3 LearValueVector3 = float3(LerpValue, LerpValue, LerpValue);
    

    float3 Albedo0 = Textures[instData.MatIndex0].Sample(gsamLinearWrap, pin.TexC);
    float3 Albedo1 = Textures[instData.MatIndex1].Sample(gsamLinearWrap, pin.TexC);
    float3 Albedo = lerp(Albedo0, Albedo1, LearValueVector3);

    float Roughness0 = Textures[instData.MatIndex0 + 1].Sample(gsamLinearWrap, pin.TexC).x;
    float Roughness1 = Textures[instData.MatIndex1 + 1].Sample(gsamLinearWrap, pin.TexC).x;
    float Roughness = lerp(Roughness0, Roughness1, LerpValue);

    float Metallic0 = Textures[instData.MatIndex0 + 2].Sample(gsamLinearWrap, pin.TexC).x;
    float Metallic1 = Textures[instData.MatIndex1 + 2].Sample(gsamLinearWrap, pin.TexC).x;
    float Metallic = lerp(Metallic0, Metallic1, LerpValue);

    float Ao0 = Textures[instData.MatIndex0 + 3].Sample(gsamLinearWrap, pin.TexC).x;
    float Ao1 = Textures[instData.MatIndex1 + 3].Sample(gsamLinearWrap, pin.TexC).x;
    float Ao = lerp(Ao0, Ao1, LerpValue);

    float3 N0 = Textures[instData.MatIndex0 + 4].Sample(gsamLinearWrap, pin.TexC);
    float3 N1 = Textures[instData.MatIndex1 + 4].Sample(gsamLinearWrap, pin.TexC);
    float3 N = lerp(N0, N1, LearValueVector3);


    //Outgoing light direction(월드 스페이스 픽셀에서 눈으로의 벡터)
    //float3 Lo = normalize(gEyePosW - pin.PosW);

    //Normal(픽셀에서의 노말)
    N = normalize(2.0 * N - 1.0);
    N = normalize(mul(N, pin.TangentBasis));


    PS_OUTPUT psOutput;

    psOutput.albedo = float4(UnGammaCorrection(Albedo), 1);
    psOutput.metallicRoughnessAo = float4(Metallic, Roughness, Ao, 1);
    psOutput.normal = float4(N, 1);

    return psOutput;
   
}


