#include"ui_structure.hlsl"

struct VertexIn
{
    float3 posL : POSITION;
    float2 uv  : TEXCOORD;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD;
    nointerpolation uint instID : INSTID;
};


VertexOut VS(VertexIn vIn, uint instanceID : SV_InstanceID)
{
    VertexOut vsOut;

    float4 position = float4(vIn.posL, 1.f);
    position = mul(position, gInstanceData[instanceID].worldProjTM);

    
    vsOut.posH = position;
    vsOut.uv = vIn.uv;
    vsOut.instID = instanceID;


    return vsOut;
}
