#include"waveRootSig.hlsl"
#include"lighting.hlsl"
struct PixelIn
{
    float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC    : TEXCOORD;
};

struct PS_OUTPUT
{
    float4 albedo : SV_Target0;
    float4 metallicRoughnessAo : SV_Target1;
    float4 normal : SV_Target2;
    float4 emissive : SV_Target3;
};

PS_OUTPUT PS(PixelIn pin)
{
   /* float4x4 textTransform =
        float4x4(
        10.f, 0.f, 0.f, 0.f,
            0.f, 10.f, 0.f, 0.f,
        0, 0, 10.f, 0,
        scene.totalTime/10, 0, 0, 1
    );*/

  //  float2 newUV = mul(float4(pin.TexC, 0, 1), textTransform).xy;

    //float3 textureColor = Texture.Sample(gsamLinearWrap, newUV.xy);

    //Normal(픽셀에서의 노말)
    float3 N = normalize(pin.NormalW);


    PS_OUTPUT psOutput;

    //psOutput.albedo = float4(UnGammaCorrection(textureColor), 1);
    psOutput.albedo = float4(1,1,1, 1);

    psOutput.metallicRoughnessAo = float4(0.8f, 0.1f, 0.9, 1);
    psOutput.normal = float4(N, 1);
    psOutput.emissive = float4(0,0,0,1);

    return psOutput;
  
}


