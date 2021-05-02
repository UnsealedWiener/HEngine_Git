#include"RasterizeRootSig.hlsl"

#define NUM_PATCH_POINTS 3

struct HullOut
{
    float3 PosW    : POSITION;
    float3x3 TangentBasis : TBASIS;
    float2 TexC    : TEXCOORD;

};

struct HullConstantOut
{
    float mEdgeFactors[3] : SV_TessFactor;
    float mInsideFactors : SV_InsideTessFactor;
    nointerpolation uint instID : INSTID;

};

struct DomainOut
{
    float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3x3 TangentBasis : TBASIS;
    float2 TexC    : TEXCOORD;
    nointerpolation uint instID : INSTID;

};

[domain("tri")]
DomainOut DS(const HullConstantOut HSConstantOutput,
    const float3 uvw : SV_DomainLocation,
    const OutputPatch <HullOut, NUM_PATCH_POINTS> patch
    )
{
    DomainOut output = (DomainOut)0;

    output.instID = HSConstantOutput.instID;

    InstanceData instData = gInstanceData[HSConstantOutput.instID];

    int matIndex0 = instData.MatIndex0;
    int matIndex1 = instData.MatIndex1;

    // Get texture coordinates
      //output.TexC = uvw.x * patch[0].TexC + uvw.y * patch[1].TexC + uvw.z * patch[2].TexC;
    output.TexC = uvw.x * patch[1].TexC + uvw.y * patch[2].TexC + uvw.z * patch[0].TexC;


    // Get normal
    //const float3x3 TBN = uvw.x * patch[0].TangentBasis + uvw.y * patch[1].TangentBasis + uvw.z * patch[2].TangentBasis;
    const float3x3 TBN = uvw.x * patch[1].TangentBasis + uvw.y * patch[2].TangentBasis + uvw.z * patch[0].TangentBasis;

    output.TangentBasis = TBN;
    float3 N = normalize(TBN[2]);

    // Get position
    // float3 PosW = uvw.x * patch[0].PosW + uvw.y * patch[1].PosW + uvw.z * patch[2].PosW;
    float3 PosW = uvw.x * patch[1].PosW + uvw.y * patch[2].PosW + uvw.z * patch[0].PosW;

    float height0 = Textures[matIndex0 + 5].SampleLevel(gsamLinearWrap, output.TexC, 0).x;
    float height1 = Textures[matIndex1 + 5].SampleLevel(gsamLinearWrap, output.TexC, 0).x;
    float height = lerp(height0, height1, instData.LerpValue);

    height = height * 2 - 1;

    float heightScale = 0.03;
    const float displacement = (heightScale * (height ));
    //const float displacement = (height );


    // Offset vertex along normal
    PosW += N * displacement;
    PosW += 0;
    output.PosW = PosW;

    output.PosH = mul(float4(PosW, 1), scene.viewProj);
 

    return output;
}

