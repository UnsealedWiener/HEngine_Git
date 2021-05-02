#include"RasterizeRootSig.hlsl"

struct VertexIn
{
	float3 PosL        : POSITION;
    float3 NormalL     : NORMAL;
    float3 TangentL    : TANGENTU;
    float3 Binorm      : BINORMV;
	float2 TexC        : TEXCOORD;
    uint4 BoneIndex   : BONEINDEX;
    float4 BlendWeight : BLENDWEIGHT;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
};

float4x4 Skinning(uint4 boneIndex, float4 blendWeight, uint boneIndexOffset)
{
    float4x4 mat0 = gInstanceBoneTM[boneIndexOffset + boneIndex.x];
    float4x4 mat1 = gInstanceBoneTM[boneIndexOffset + boneIndex.y];
    float4x4 mat2 = gInstanceBoneTM[boneIndexOffset + boneIndex.z];
    float4x4 mat3 = gInstanceBoneTM[boneIndexOffset + boneIndex.w];


    float4x4 result =
        mat0 * blendWeight.x +
        mat1 * blendWeight.y +
        mat2 * blendWeight.z +
        mat3 * blendWeight.w;

    return result;
}

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
	VertexOut vout = (VertexOut)0.0f;
	
	// Fetch the instance data.
	InstanceData instData = gInstanceData[instanceID];

    float4x4 skinTM = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    if(gBoneCount != 0)
        skinTM = Skinning(vin.BoneIndex, vin.BlendWeight, (gPsoData[0].instanceNumOffset + instanceID) * gBoneCount);
	float4x4 world = instData.World;

    world = mul(mul(skinTM, gAxisChange), world);

    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    float4 posV = mul(posW, scene.lightView);
    float4 posP = mul(posV, scene.lightProj);



   vout.PosH = posP;
    //vout.PosH = mul(mul(posW, gView), gProj);

	
    return vout;
}
