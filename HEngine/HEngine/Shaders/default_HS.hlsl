#include"RasterizeRootSig.hlsl"

#define NUM_PATCH_POINTS 3

struct HullIn
{
    float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3x3 TangentBasis : TBASIS;
    float2 TexC    : TEXCOORD;
    float TessllationFactor : TESS;
    // nointerpolation is used so the index is not interpolated 
    // across the triangle.
    nointerpolation uint instID : INSTID;
};

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

HullConstantOut constant_hull_shader(const InputPatch<HullIn, NUM_PATCH_POINTS> patch,
    const uint patchID : SV_PrimitiveID)
{
    // Average tess factors along edges, and pick an edge tess factor for 
    // the interior tessellation. It is important to do the tess factor
    // calculation based on the edge properties so that edges shared by 
    // more than one triangle will have the same tessellation factor.  
    // Otherwise, gaps can appear.
    HullConstantOut output = (HullConstantOut)0;
    //output.mEdgeFactors[0] = 0.5f * (patch[1].TessllationFactor + patch[2].TessllationFactor);
    //output.mEdgeFactors[1] = 0.5f * (patch[2].TessllationFactor + patch[0].TessllationFactor);
    //output.mEdgeFactors[2] = 0.5f * (patch[0].TessllationFactor + patch[1].TessllationFactor);

   output.mEdgeFactors[0] = 0.5f * (patch[0].TessllationFactor + patch[2].TessllationFactor);
    output.mEdgeFactors[1] = 0.5f * (patch[0].TessllationFactor + patch[1].TessllationFactor);
    output.mEdgeFactors[2] = 0.5f * (patch[1].TessllationFactor + patch[2].TessllationFactor);


    output.mInsideFactors = output.mEdgeFactors[0];

    output.instID = patch[0].instID;
   
    return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_PATCH_POINTS)]
[patchconstantfunc("constant_hull_shader")]
HullOut HS(const InputPatch <HullIn, NUM_PATCH_POINTS> patch,
    const uint controlPointID : SV_OutputControlPointID,
    const uint patchId : SV_PrimitiveID)
{
    HullOut output = (HullOut)0;
    output.PosW = patch[controlPointID].PosW;
    output.TangentBasis = patch[controlPointID].TangentBasis;
    output.TexC = patch[controlPointID].TexC;
    
    return output;
}

