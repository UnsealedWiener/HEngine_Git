#include"lighting.hlsl"
#include"SceneVariables.hlsl"


ConstantBuffer<sceneVariables> scene : register(b0);
StructuredBuffer<Light> gLights : register(t0, space1);

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


RWTexture2D<float4> resultBuffer0 : register(u0);
//RWTexture2D<float4> resultBuffer1 : register(u1);


//0 = albedo
//1 = metallicRoughnessAo
//2 = worldNormal
//3 = depth
Texture2D albedoTexture:    register(t0);
Texture2D metallicRoughnessAOTexture:    register(t1);
Texture2D worldNormalTexture:    register(t2);
Texture2D emissiveTexture:    register(t3);
Texture2D depthTexture:    register(t4);
Texture2D reflectionTexture:    register(t5);
Texture2D shadowTexture:    register(t6);
Texture2D ssaoTexture:    register(t7);


SamplerComparisonState gsamShadow : register(s6);

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
	float3 albedo = float3(albedoTexture[dispatchThreadID.xy].xyz);
	float metallic = metallicRoughnessAOTexture[dispatchThreadID.xy].x;
	float roughness = metallicRoughnessAOTexture[dispatchThreadID.xy].y;
	float ao = metallicRoughnessAOTexture[dispatchThreadID.xy].z;
	float3 emissive = emissiveTexture[dispatchThreadID.xy];
	float3 normal = float3(worldNormalTexture[dispatchThreadID.xy].xyz);
	float depth = depthTexture[dispatchThreadID.xy].x;
	float3 reflect = float3(reflectionTexture[dispatchThreadID.xy].xyz);
	float ssao = float(ssaoTexture[dispatchThreadID.xy].x);

	ao = min(ao, ssao);

	float3 color = { 0,0,0 };
	if (depth < 1.f)
	{
		float4 ndcCoord = { 2 * (dispatchThreadID.x / scene.renderTargetSize.x) - 1, 1 - 2 * (dispatchThreadID.y / scene.renderTargetSize.y), depth, 1 };
		float4 viewCoord = mul(ndcCoord, scene.invProj);
		viewCoord = viewCoord / viewCoord.w;
		float4 worldCoord = mul(viewCoord, scene.invView);

		float4 shadowCoord = mul(worldCoord, scene.shadowTransform);
		float isShadowOn = shadowTexture.SampleCmpLevelZero(gsamShadow, shadowCoord.xy, shadowCoord.z).r;

		//Outgoing light direction(???? ???????? ???????? ???????? ????)
		float3 Lo = normalize(scene.eyePosW - float3(worldCoord.xyz));

		//?????? Normal?? Outgoing light?? ????
		float cosLo = max(0.0, dot(normal, Lo));

		//Specular reflection vector(???? ?????? ?? ??????????, Normal?? Lo?? ?????? ?????????? ???? ???? ?????? ???? ?????? ???? ?? ????
		float3 Lr = 2.0 * cosLo * normal - Lo;

		//Frensnel reflentance at normal incidence(for metals use albedo color)
		//?????? ?????? ???? ?????? ?? ?????? ?????? ????
		float3 Fdielectric = float3(0.04, 0.04, 0.04);//??????
		float3 F0 = lerp(Fdielectric, albedo, metallic);

		float3 directLighting = 0.0f;
		int lightIndex = 0;
		for (int i = 0; i < scene.dirLightCnt; i++, lightIndex++)
		{
			float3 lightStrength = abs(gLights[lightIndex].strength);

			directLighting += DirLightCalculation(gLights[lightIndex].direction,
				lightStrength, Lo, normal, cosLo, F0, albedo, metallic,
				roughness, isShadowOn);
		}

		float3 pointLighting = 0.0f;
		for (uint i = 0; i < scene.pointLightCnt; i++, lightIndex++)
		{
			float3 lightStrength = abs(gLights[lightIndex].strength);

			pointLighting += PointLightCalculation(gLights[lightIndex].position,
				lightStrength, gLights[lightIndex].falloffStart,
				gLights[lightIndex].falloffEnd, worldCoord.xyz,
				Lo, normal, cosLo, F0, albedo, metallic, roughness);
		}

		float3 spotLighting = 0.0f;

		for (uint i = 0; i < scene.spotLightCnt; i++, lightIndex++)
		{

			float3 lightStrength = abs(gLights[lightIndex].strength);

			spotLighting += SpotLightCalculation(gLights[lightIndex].position,
				gLights[lightIndex].direction, lightStrength,
				gLights[lightIndex].falloffStart, gLights[lightIndex].falloffEnd,
				gLights[lightIndex].spotPower, worldCoord.xyz,
				Lo, normal, cosLo, F0, albedo, metallic, roughness);
		}

		float3 indirectLighting = 0;
		{
			float3 irradiance = float3(reflect.xyz);

			float3 F = FresnelSchlick(F0, cosLo);

			float3 kd = lerp(1.0 - F, 0.0f, metallic);

			// Irradiance map contains exitant radiance assuming Lambertian BRDF, no need to scale by 1/PI here either.
			float3 diffuseIBL = kd * albedo * irradiance;

			// Sample pre-filtered specular reflection environment at correct mipmap level.
			//uint specularTextureLevels = querySpecularTextureLevels();
			//float3 specularIrradiance = specularTexture.SampleLevel(defaultSampler, Lr, roughness * specularTextureLevels).rgb;

			//float3 metallicFactor = lerp

			float3 specularIBL = F0 * irradiance;

			//// Split-sum approximation factors for Cook-Torrance specular BRDF.
			//float2 specularBRDF = specularBRDF_LUT.Sample(spBRDF_Sampler, float2(cosLo, roughness)).rg;

			//// Total specular IBL contribution.
			//float3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

			// Total ambient lighting contribution.
			indirectLighting = diffuseIBL + specularIBL;
		}

		float3 Ambient = float3(0.2f, 0.2f, 0.2f) * indirectLighting * ao;

		color = directLighting + pointLighting + spotLighting + Ambient;
	}
	else
	{
		if (dot(reflect.xyz, float3(1.f, 1.f, 1.f)) == 0)
			color = float3(0.5f, 0.5f, 0.5f);
		else
			color = reflect.xyz;
	}

	color += emissive;
	//float Gamma = 1.0f / 2.2f;
	//color = color / (color + float3(1.0f, 1.0f, 1.0f));
	//color = pow(color, float3(Gamma, Gamma, Gamma));

	//float tempshadow = float(shadowTexture[dispatchThreadID.xy].x);

	float4 finalResult = float4(GammaCorrection(color), 1);

	if (isnan(finalResult).x)
		finalResult = 0.f;

	resultBuffer0[dispatchThreadID.xy] = finalResult;
}


//
//
//// Shlick's approximation of the Fresnel factor.
//float3 FresnelSchlick(float3 F0, float cosTheta)
//{
//	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
//}
//
//// GGX/Towbridge-Reitz normal distribution function.
//// Uses Disney's reparametrization of alpha = roughness^2.
//float NDFGGX(float cosLh, float roughness)
//{
//	float alpha = roughness * roughness;
//	float alphaSq = alpha * alpha;
//
//	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
//	return alphaSq / (PI * denom * denom);
//}
//
//// Single term for separable Schlick-GGX below.
//float GASchlickG1(float cosTheta, float k)
//{
//	return cosTheta / (cosTheta * (1.0 - k) + k);
//}
//
//
//// Schlick-GGX approximation of geometric attenuation function using Smith's method.
//float GASchlickGGX(float cosLi, float cosLo, float roughness)
//{
//	float r = roughness + 1.0;
//	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
//	return GASchlickG1(cosLi, k) * GASchlickG1(cosLo, k);
//}
//
//

