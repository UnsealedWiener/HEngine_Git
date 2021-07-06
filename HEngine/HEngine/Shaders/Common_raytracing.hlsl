#include"SceneVariables.hlsl"
#include"lighting.hlsl"

#define MAX_RAY_RECURSION_DEPTH 3
#define MAX_MIPMAPLEVEL 8

ConstantBuffer<sceneVariables> scene : register(b0);
RaytracingAccelerationStructure gSceneAS : register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

//쉐이더 내부에서 모든 쉐이더 함수와 통신할 기초 구조체를 정의한다.
struct Payload
{
	float4 color;
	uint recursionDepth;
	float mipmapLevel;
};

struct Ray
{
	float3 origin;
	float3 direction;
};

// Trace a radiance ray into the scene and returns a shaded color.
float4 TraceRadianceRay(in Ray ray, in uint currentRayRecursionDepth, in float mipmapLevel)
{
	if (currentRayRecursionDepth == MAX_RAY_RECURSION_DEPTH)
	{
		return float4(0, 0, 0, 0);
	}

	// Set the ray's extents.
	RayDesc rayDesc;
	rayDesc.Origin = ray.origin;
	rayDesc.Direction = ray.direction;
	// Set TMin to a zero value to avoid aliasing artifacts along contact areas.
	// Note: make sure to enable face culling so as to avoid surface face fighting.
	rayDesc.TMin = scene.nearZ;
	rayDesc.TMax = scene.farZ;

	Payload rayPayload = { float4(0, 0, 0, 0), currentRayRecursionDepth + 1, mipmapLevel };

	TraceRay(
		gSceneAS,		//AccelerationStructure
		RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,	//RayFlags
		0xFF,			//InstanceInculsionMask
		0,				//RayContributionToHitGroupIndex
		0,				//MultiplierForGeometryContributionToHitGroupIndex
		0,				//MissShaderIndex
		rayDesc,
		rayPayload
	);

	return rayPayload.color;
}

float4 TraceRayAccoringToRoughness(float3 surfaceNormal, float3 rayStartPosition, float3 rayDirection,
	float roughness , uint recursionDepth)
{
	if (recursionDepth == MAX_RAY_RECURSION_DEPTH)
	{
		return float4(0, 0, 0, 0);
	}

	Ray mainRay;
	mainRay.origin = rayStartPosition;
	mainRay.direction = rayDirection;



}