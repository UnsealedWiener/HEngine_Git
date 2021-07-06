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

TextureCube Skybox      : register(t0, space1);

[shader("miss")]
void Miss(inout Payload payload : SV_Payload)
{
	float mipmapLevel = payload.mipmapLevel;

	float3 skyboxColor = Skybox.SampleLevel(gsamLinearWrap, WorldRayDirection(), mipmapLevel).xyz;

	if (skyboxColor.x == 0.f && skyboxColor.x == 0.f && skyboxColor.x == 0.f)
	{
		payload.color = float4(0.5f, 0.5f, 0.5f,1.f);
		return;
	}

	payload.color = float4(UnGammaCorrection(skyboxColor),1);
}