#include"waveRootSig.hlsl"

struct VertexIn
{
	float3 PosL        : POSITION;
    float3 NormalL     : NORMAL;
	float2 TexC        : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    float4 posW = mul(float4(vin.PosL, 1.0f), constData.world);
    vout.PosW = posW.xyz;

    float3 normalW = mul(float4(vin.NormalL, 0), constData.worldInvTrans);
    vout.NormalW = normalW;

    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), constData.texTransform);
    vout.TexC = texC.xy;

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, scene.viewProj);
	
    return vout;
}
