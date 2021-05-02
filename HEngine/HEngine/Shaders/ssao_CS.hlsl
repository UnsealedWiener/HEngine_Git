
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

struct Light
{
	float3 Strength;
	float FalloffStart; // point/spot light only
	float3 Direction;   // directional/spot light only
	float FalloffEnd;   // point/spot light only
	float3 Position;    // point light only
	float SpotPower;    // spot light only
};

#define MaxLights 16

cbuffer cbPass : register(b0)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float4x4 gLightView;
	float4x4 gLightProj;
	float4x4 gShadowTransform;
	float3 gEyePosW;
	float cbPerObjectPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float4 gAmbientLight;

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light gLights[MaxLights];

	bool isPerspective;
	uint gpadding[3];
};


RWTexture2D<float4> resultBuffer : register(u0);

//0 = albedo
//1 = metallicRoughnessAo
//2 = worldNormal
//3 = depth
Texture2D depthTexture:    register(t0);
Texture2D worldNormalTexture:    register(t1);
Texture2D randomVectorTexture:    register(t2);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamLinearWrap       : register(s2);
SamplerComparisonState gsamShadow : register(s6);

static const float PI = 3.141592;
static const float Epsilon = 0.00001;

static const float gOcclusionRadius = 0.5f*20.f;
static const float gSurfaceEpsilon = 0.05f*100.f;
static const float gOcclusionFadeEnd = 1.0f*80.f ;
static const float gOcclusionFadeStart = 0.2f*50.f;

#define sampleCount 14

static const float4 offsetVectors[sampleCount] =
{
	float4(1.0f, 1.0f, 1.0f, 0.0f),
	float4(-1.0f, -1.0f, -1.0f, 0.0f),
	float4(-1.0f, 1.0f, 1.0f, 0.0f),
	float4(1.0f, -1.0f, -1.0f, 0.0f),
	float4(1.0f, 1.0f, -1.0f, 0.0f),
	float4(-1.0f, -1.0f, 1.0f, 0.0f),
	float4(-1.0f, 1.0f, -1.0f, 0.0f),
	float4(1.0f, -1.0f, 1.0f, 0.0f),
	float4(-1.0f, 0.0f, 0.0f, 0.0f),
	float4(1.0f, 0.0f, 0.0f, 0.0f),
	float4(0.0f, 1.0f, 0.0f, 0.0f),
	float4(0.0f, -1.0f, 0.0f, 0.0f),
	float4(0.0f, 0.0f, 1.0f, 0.0f),
	float4(0.0f, 0.0f, -1.0f, 0.0f),
};

[numthreads(16, 16, 1)]
void CS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	float2 normalizedUv = { dispatchThreadID.x / gRenderTargetSize.x, dispatchThreadID.y / gRenderTargetSize.y };

	float3 normal = float3(worldNormalTexture[dispatchThreadID.xy].xyz);
	float3 randomVector = float3(randomVectorTexture.SampleLevel(gsamLinearWrap, 8*normalizedUv, 0).xyz);
	randomVector = 2 * randomVector - 1.0f;
	float depth = depthTexture[dispatchThreadID.xy].x;

	//view ³ë¸»
	float3 viewNormal = mul(normal, gView);

	//view ÁÂÇ¥
	float4 ndcCoord = { 2 * normalizedUv.x - 1, 1 - 2 * normalizedUv.y, depth, 1 };
	float4 viewCoord = mul(ndcCoord, gInvProj);
	viewCoord = viewCoord / viewCoord.w;

	float occlusionSum = 0;

	for (int i = 0; i < sampleCount; i++)
	{
		float3 offset = reflect(offsetVectors[i].xyz, randomVector);

		float flip = sign(dot(offset, viewNormal));

		float3 q = viewCoord + flip * gOcclusionRadius * offset;

		float4 projQ = mul(float4(q, 1.0f), gProj);
		projQ /= projQ.w;
		int2 textureCoordQ = { gRenderTargetSize.x * (projQ.x + 1) / 2 , gRenderTargetSize.y * (-projQ.y + 1) / 2 };
		float rz = depthTexture[textureCoordQ.xy].x;

		projQ = float4(projQ.x, projQ.y, rz, 1);

		float4 viewQ = mul(projQ, gInvProj);
		viewQ = viewQ / viewQ.w;

		float distZ = viewCoord.z - viewQ.z;
		float dp = max(dot(viewNormal, normalize(float3(viewQ.xyz - viewCoord.xyz))), 0.0f);
		float occlusion = 0.0f;
		if (distZ > gSurfaceEpsilon)
		{
			float fadeLength = gOcclusionFadeEnd - gOcclusionFadeStart;
			occlusion = saturate((gOcclusionFadeEnd - distZ) / fadeLength);
		}

		occlusion = occlusion * dp;
		occlusionSum += occlusion;

	}

	occlusionSum /= sampleCount;

	float access = 1.0f - occlusionSum;

	
	resultBuffer[dispatchThreadID.xy].x = saturate(pow(access, 4.0f));
}




