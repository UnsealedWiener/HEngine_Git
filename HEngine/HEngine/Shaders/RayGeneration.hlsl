#include"Common_raytracing.hlsl"

//defined in "Common_raytracing.hlsl"
//local root signature should not overlap same register.
/*
* list of global root signature
* 
ConstantBuffer<sceneVariables> scene : register(b0);
RaytracingAccelerationStructure gSceneAS : register(t0);
SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
*/

RWTexture2D<float4> gColorOutput : register(u0);
Texture2D normal_gbuffer : register(t0, space1);
Texture2D depth_gbuffer : register(t1, space1);
Texture2D metallicRoughnessAoEmissive : register(t2, space1);
Texture2D randomTexture : register(t3, space1);


static const float4 offsetVectors[14] =
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

[shader("raygeneration")]
void RayGeneration()
{
	uint2 launchIndex = DispatchRaysIndex().xy;
	uint2 dimensions = DispatchRaysDimensions().xy;

	float2 ndc = ((((float2)launchIndex + 0.5f) / (float2)dimensions) * 2.f - 1.f);
	float2 uv = ((float2)launchIndex + 0.5f) / (float2)dimensions;

	Payload payload;

	float3 rayOrigin;
	float3 rayDirection;

	float3 rayOrigin_fromCamera;
	float3 rayDirection_fromCamera;


	//Perspective
	if (scene.isPerspective)
	{
		rayOrigin_fromCamera = mul(float4(0, 0, 0, 1), scene.invView).xyz;
		float4 target = mul(float4(ndc.x, -ndc.y, 0, 1), scene.invProj);
		//target = float4(target.x / target.w, target.y / target.w, target.z / target.w, target.w / target.w);
		rayDirection_fromCamera = mul(float4(target.xyz, 0), scene.invView).xyz;
		rayDirection_fromCamera = normalize(rayDirection_fromCamera).xyz;
	}
	else//orthographic
	{
		float4 pixelInWorld = mul(float4(ndc.x, -ndc.y, 0, 1), scene.invProj);
		pixelInWorld = pixelInWorld / pixelInWorld.w;
		pixelInWorld = mul(pixelInWorld, scene.invView);
		rayOrigin_fromCamera = float3(pixelInWorld.xyz);
		rayDirection_fromCamera = mul(float4(0, 0, 1, 0), scene.invView).xyz;

	}

	float3 normal = normal_gbuffer[launchIndex.xy].xyz;
	float depth = depth_gbuffer[launchIndex.xy].x;
	float roughness = metallicRoughnessAoEmissive[launchIndex.xy].y;
	float3 randomVector =
		randomTexture.SampleLevel(gsamLinearWrap, uv, 0).xyz;
	

	if (depth == 1.0f)
	{
		rayOrigin = rayOrigin_fromCamera;
		rayDirection = rayDirection_fromCamera;

		Ray ray = { rayOrigin, rayDirection };

		uint currentRecursionDepth = 0;
		float mipmapLevel = 0.0f;

		float4 color = TraceRadianceRay(ray, currentRecursionDepth, mipmapLevel);

		gColorOutput[launchIndex] = color;

	}
	else
	{
		if (roughness < 0.3)
		{
			float4 viewCoord = mul(float4(ndc.x, -ndc.y, depth, 1), scene.invProj);
			viewCoord = viewCoord / viewCoord.w;
			float4 worldCoord = mul(viewCoord, scene.invView);

			uint currentRecursionDepth = 0;
			float mipmapLevel = 0.0f;

			rayOrigin = float3(worldCoord.xyz);
			rayDirection = reflect(rayDirection_fromCamera, normal);

			Ray mainRay = { rayOrigin, rayDirection };

			float4 mainRayColor =
				TraceRadianceRay(mainRay, currentRecursionDepth, mipmapLevel);

			gColorOutput[launchIndex] = mainRayColor;
		}
		else
		{
			gColorOutput[launchIndex] = float4(0.5,0.5,0.5,1);
		}
		
	}

	
}

