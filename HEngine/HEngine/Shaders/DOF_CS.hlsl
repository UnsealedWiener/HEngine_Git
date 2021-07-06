
#include"SceneVariables.hlsl"


struct DOFParam
{
	float focusDepth;
	float maxBlurDepthGap;
	int padding[2];
};

ConstantBuffer<sceneVariables> scene : register(b0);
ConstantBuffer<DOFParam> gDOFParam : register(b1);

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

Texture2D sceneTexture:    register(t0);
Texture2D depthTexture:    register(t1);
Texture2D blurredTexture:    register(t2);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

//SamplerComparisonState gsamShadow : register(s6);

//static const float PI = 3.141592;
//static const float Epsilon = 0.00001;
//
//float3 FresnelSchlick(float3 F0, float cosTheta);
//float NDFGGX(float cosLh, float roughness);
//float GASchlickG1(float cosTheta, float k);
//float GASchlickGGX(float cosLi, float cosLo, float roughness);

[numthreads(16, 16, 1)]
void CS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	float2 textureCoord = { dispatchThreadID.x / scene.renderTargetSize.x , dispatchThreadID.y / scene.renderTargetSize.y };

	float4 originScene = sceneTexture.SampleLevel(gsamLinearWrap, textureCoord, 0);
	float4 blurredScene = blurredTexture.SampleLevel(gsamLinearWrap, textureCoord, 0);
	float depth  = depthTexture.SampleLevel(gsamLinearWrap, textureCoord, 0);

	float focusDepth = gDOFParam.focusDepth;
	float maxBlurGap = gDOFParam.maxBlurDepthGap;


	float depthGap = abs(depth - focusDepth);
	depthGap = saturate(depthGap / maxBlurGap);

	depthGap = depthGap * depthGap;

	float4 color = lerp(originScene, blurredScene, depthGap);

	//resultBuffer[dispatchThreadID.xy] = float4(originScene.xyz, 1);

	resultBuffer[dispatchThreadID.xy] = float4(color.xyz, 1);

}

