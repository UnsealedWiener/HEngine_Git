
#include"SceneVariables.hlsl"

ConstantBuffer<sceneVariables> scene : register(b0);

//cbuffer cbPass : register(b0)
//{
//	float4x4 gView;
//	float4x4 gInvView;
//	float4x4 gProj;
//	float4x4 gInvProj;
//	float4x4 gViewProj;
//	float4x4 gInvViewProj;
//	float4x4 gLightView;
//	float4x4 gLightProj;
//	float4x4 gShadowTransform;
//	float3 gEyePosW;
//	float cbPerObjectPad1;
//	float2 gRenderTargetSize;
//	float2 gInvRenderTargetSize;
//	float gNearZ;
//	float gFarZ;
//	float gTotalTime;
//	float gDeltaTime;
//	float4 gAmbientLight;
//
//	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
//	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
//	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
//	// are spot lights for a maximum of MaxLights per object.
//	Light gLights[MaxLights];
//
//	bool isPerspective;
//	uint gpadding[3];
//};

RWTexture2D<float4> resultBuffer : register(u0);

Texture2D originTexture:    register(t0);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

static float gGaussianWeights[11] =
{
   0.009300040045324049,
   0.028001560233780885,
   0.06598396774984912,
   0.12170274650962626,
   0.17571363439579307,
   0.19859610213125314,
   0.17571363439579307,
   0.12170274650962626,
   0.06598396774984912,
   0.028001560233780885,
   0.009300040045324049
};
static const int g_blurRadius = 5;
#define N 256
#define CacheSize (g_blurRadius*2 + 256)
groupshared float4 gCache[CacheSize];

[numthreads(1, N, 1)]
void CS(int3 groupThreadID : SV_GroupThreadID,
	int3 dispatchThreadID : SV_DispatchThreadID)
{
	uint2 inputTextureSize;
	uint numberOfLevels;

	originTexture.GetDimensions(0, inputTextureSize.x, inputTextureSize.y, numberOfLevels);

	
	float2 quaterSize = scene.renderTargetSize / 2;

	//2*g_blurRadius만큼의 스레드가 픽셀을 하나씩 더 추출한다
	if (groupThreadID.y < g_blurRadius)
	{
		//이미지 가장자리로 한정
		int y = max(dispatchThreadID.y - g_blurRadius, 0);
		
		float2 sampleCoord =
			float2(dispatchThreadID.x, y) / quaterSize;
		gCache[groupThreadID.y] = originTexture.SampleLevel(gsamLinearClamp, sampleCoord,0);

	}
	if (groupThreadID.y >= N - g_blurRadius)
	{
		int y = min(dispatchThreadID.y + g_blurRadius, quaterSize.y - 1);


		float2 sampleCoord =
			float2(dispatchThreadID.x, y) / quaterSize;


		gCache[groupThreadID.y + 2* g_blurRadius] = originTexture.SampleLevel(gsamLinearClamp, sampleCoord, 0);

	}

	float2 sampleCoord =
		float2(dispatchThreadID.xy) / quaterSize;

	//gCache[groupThreadID.y + g_blurRadius] = originTexture[min(dispatchThreadID.xy, inputTextureSize.xy - 1)];
	gCache[groupThreadID.y + g_blurRadius] 
		= originTexture.SampleLevel(gsamLinearClamp, min(sampleCoord, quaterSize.xy - 1), 0);


	GroupMemoryBarrierWithGroupSync();


	float4 blurColor = float4(0, 0, 0, 0);

	for (int i = -g_blurRadius; i <= g_blurRadius; i++)
	{
		int k = groupThreadID.y + g_blurRadius + i;

		blurColor += gGaussianWeights[i + g_blurRadius] * gCache[k];
	}

	resultBuffer[dispatchThreadID.xy] = blurColor;

}

