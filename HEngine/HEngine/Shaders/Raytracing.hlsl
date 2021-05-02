

#define MAX_RAY_RECURSION_DEPTH 3

#define MAX_MIPMAPLEVEL 8

#include"lighting.hlsl"
#include"SceneVariables.hlsl"

struct Vertex_Static
{
	float3 pos;
	float3 normal;
	float3 tangentU;
	float3 binormV;
	float2 texC;
	float4 boneIndex;
	float4 blendWeight;
};

struct Vertex_Dynamic
{
	float3 pos;
	float3 normal;
	float3 tangentU;
	float3 binormV;
	float2 texC;
	int	   padding[2];
};

struct Vertex
{
	float3 pos;
	float3 normal;
	float3 tangentU;
	float3 binormV;
	float2 texC;
	int	   padding[2];
};

Vertex VertexToVertex(Vertex_Static staticVertex)
{
	Vertex resultVertex;
	resultVertex.pos = staticVertex.pos;
	resultVertex.normal = staticVertex.normal;
	resultVertex.tangentU = staticVertex.tangentU;
	resultVertex.binormV = staticVertex.binormV;
	resultVertex.texC = staticVertex.texC;
	return resultVertex;
}

Vertex VertexToVertex(Vertex_Dynamic dynamicVertex)
{
	Vertex resultVertex;
	resultVertex.pos = dynamicVertex.pos;
	resultVertex.normal = dynamicVertex.normal;
	resultVertex.tangentU = dynamicVertex.tangentU;
	resultVertex.binormV = dynamicVertex.binormV;
	resultVertex.texC = dynamicVertex.texC;
	return resultVertex;
}


struct RaytracingDataPerInstance
{
	uint modelIndex;
	uint instanceIndexInModel;
	uint matIndex0;
	uint matIndex1;
	float lerpValue;
	uint padding[3];
};

struct RaytracingDataPerModel
{
	uint vertexOffset;
	uint indexOffset;
	uint vertexCount;
	bool isDynamicModel;
};


ConstantBuffer<sceneVariables> scene : register(b0);
StructuredBuffer<Light> gLights : register(t5, space1);


RWTexture2D<float4> gColorOutput : register(u0);
RaytracingAccelerationStructure gSceneAS : register(t0);

ByteAddressBuffer g_indices : register(t0, space1);
StructuredBuffer<Vertex_Static> g_vertices : register(t1, space1);
StructuredBuffer<Vertex_Dynamic> g_vertices_dynamic : register(t4, space1);

StructuredBuffer<RaytracingDataPerInstance> g_instanceData : register(t2, space1);
StructuredBuffer<RaytracingDataPerModel> g_modelsData : register(t3, space1);



// An array of textures, which is only supported in shader model 5.1+.  Unlike Texture2DArray, the textures
// in this array can be different sizes and formats, making it more flexible than texture arrays.
//MatIndex + 0 = albedo
//MatIndex + 1 = roughness
//MatIndex + 2 = metallic
//MatIndex + 3 = ao
//MatIndex + 4 = normal
//MatIndex + 5 = height

Texture2D Textures[300]:    register(t0, space2);
TextureCube Skybox      : register(t300, space2);
Texture2D normal_gbuffer : register(t0, space3);
Texture2D depth_gbuffer : register(t1, space3);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

//���̴� ���ο��� ��� ���̴� �Լ��� ����� ���� ����ü�� �����Ѵ�.
struct Payload
{
	float4 color;
	uint recursionDepth;
	float mipmapLevel;
};

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
//�������� ���� ��� DirectX Raytracing API�� �ڵ����� �����ϴ�
//��Ʈ�� �ﰢ���� Ư���� �����մϴ�.
struct Build_in_attribute
{
	float BaryX;
	float BaryY;
};

struct Ray
{
	float3 origin;
	float3 direction;
};

// Trace a radiance ray into the scene and returns a shaded color.
float4 TraceRadianceRay(in Ray ray, in uint currentRayRecursionDepth, in float mipmapLevel)
{
	if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
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
	
	//rayDesc.TMin = 0;
	//rayDesc.TMax = 1000000;

	//rayDesc.TMin = 0;
	//rayDesc.TMax = 100;

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


[shader("raygeneration")]
void RayGeneration()
{
	uint2 launchIndex = DispatchRaysIndex().xy;
	uint2 dimensions = DispatchRaysDimensions().xy;

	float2 ndc = ((((float2)launchIndex + 0.5f) / (float2)dimensions) * 2.f - 1.f);

	Payload payload;

	float3 rayOrigin;
	float3 rayDirection;

	float3 rayOrigin_fromCamera;
	float3 rayDirection_fromCamera;


	//Perspective
	if (scene.isPerspective)
	{
		rayOrigin_fromCamera = mul(float4(0, 0, 0, 1), scene.invView);
		float4 target = mul(float4(ndc.x, -ndc.y, 0, 1), scene.invProj);
		//target = float4(target.x / target.w, target.y / target.w, target.z / target.w, target.w / target.w);
		rayDirection_fromCamera = mul(float4(target.xyz, 0), scene.invView);
		rayDirection_fromCamera = normalize(rayDirection_fromCamera);
	}
	else//orthographic
	{
		float4 pixelInWorld = mul(float4(ndc.x, -ndc.y, 0, 1), scene.invProj);
		pixelInWorld = pixelInWorld / pixelInWorld.w;
		pixelInWorld = mul(pixelInWorld, scene.invView);
		rayOrigin_fromCamera = float3(pixelInWorld.xyz);
		rayDirection_fromCamera = mul(float4(0, 0, 1, 0), scene.invView);

	}

	float3 normal = normal_gbuffer[launchIndex.xy].xyz;
	float depth = depth_gbuffer[launchIndex.xy].x;

	if (depth == 1.0f)
	{
		rayOrigin = rayOrigin_fromCamera;
		rayDirection = rayDirection_fromCamera;
	}
	else
	{
		float4 viewCoord = mul(float4(ndc.x, -ndc.y, depth, 1), scene.invProj);
		viewCoord = viewCoord / viewCoord.w;
		float4 worldCoord = mul(viewCoord, scene.invView);

		rayOrigin = float3(worldCoord.xyz);
		rayDirection = reflect(rayDirection_fromCamera, normal);
	}





	Ray ray = { rayOrigin, rayDirection };
	
	uint currentRecursionDepth = 0;
	float mipmapLevel = 0.0f;

	float4 color = TraceRadianceRay(ray, currentRecursionDepth, mipmapLevel);

	gColorOutput[launchIndex] = color;
	//gColorOutput[launchIndex] = float4(depth,0,0, 1);

	//if (depth == 1.0f)
	//{
	//	gColorOutput[launchIndex] = float4(1, 0, 0, 1);
	//}
	//else
	//{
	//	gColorOutput[launchIndex] = float4(0, 0, 0, 1);

	//}

}

//used when index buffer format is uint16bit
uint3 Load3x16BitIndices(uint offsetBytes)
{
	uint3 indices;

	// ByteAdressBuffer loads must be aligned at a 4 byte boundary.
	// Since we need to read three 16 bit indices: { 0, 1, 2 } 
	// aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
	// we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
	// based on first index's offsetBytes being aligned at the 4 byte boundary or not:
	//  Aligned:     { 0 1 | 2 - }
	//  Not aligned: { - 0 | 1 2 }
	const uint dwordAlignedOffset = offsetBytes & ~3;
	const uint2 four16BitIndices = g_indices.Load2(dwordAlignedOffset);

	// Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
	if (dwordAlignedOffset == offsetBytes)
	{
		indices.x = four16BitIndices.x & 0xffff;
		indices.y = (four16BitIndices.x >> 16) & 0xffff;
		indices.z = four16BitIndices.y & 0xffff;
	}
	else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
	{
		indices.x = (four16BitIndices.x >> 16) & 0xffff;
		indices.y = four16BitIndices.y & 0xffff;
		indices.z = (four16BitIndices.y >> 16) & 0xffff;
	}

	return indices;
}

uint3 Load3x32BitIndices(uint offsetBytes)
{
	const uint3 three32BitIndices = g_indices.Load3(offsetBytes);

	return three32BitIndices;
}

float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3x3 HitAttribute(float3x3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
	return vertexAttribute[0] +
		attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
		attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

float2 HitAttribute(float2 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
	return vertexAttribute[0] +
		attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
		attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}
//
//static const float PI = 3.141592;
//static const float Epsilon = 0.00001;
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
//// Schlick-GGX approximation of geometric attenuation function using Smith's method.
//float GASchlickGGX(float cosLi, float cosLo, float roughness)
//{
//	float r = roughness + 1.0;
//	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
//	return GASchlickG1(cosLi, k) * GASchlickG1(cosLo, k);
//}

float3 SampleTextureByFloat(Texture2D inputTexture, float2 texCoord, float value)
{
	uint width;
	uint height;
	uint mipmapNum;

	inputTexture.GetDimensions(0, width, height, mipmapNum);

	return inputTexture.SampleLevel(gsamLinearWrap, texCoord.xy, mipmapNum * value);
}

[shader("closesthit")]
void Hit(inout Payload payload : SV_Payload, in MyAttributes attr)
{
	float3 hitPosition = HitWorldPosition();

	RaytracingDataPerInstance instanceData = g_instanceData[InstanceID()];
	RaytracingDataPerModel modelData = g_modelsData[instanceData.modelIndex];

	uint baseVertex = modelData.vertexOffset; //nodeData.vertexOffsetInModel;
	uint baseIndex = modelData.indexOffset; //nodeData.indexOffsetInModel;

	uint indexSizeInBytes = 4;
	uint indicesPerTriangle = 3;
	uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;

	uint baseIndexInBytes = baseIndex * indexSizeInBytes + PrimitiveIndex() * triangleIndexStride;

	const uint3 indices = Load3x32BitIndices(baseIndexInBytes);

	Vertex vertex0;
	Vertex vertex1;
	Vertex vertex2;

	if (modelData.isDynamicModel == true)
	{
		vertex0 = VertexToVertex(g_vertices_dynamic[indices[0] + baseVertex +instanceData.instanceIndexInModel * modelData.vertexCount]);
		vertex1 = VertexToVertex(g_vertices_dynamic[indices[1] + baseVertex +instanceData.instanceIndexInModel * modelData.vertexCount]);
		vertex2 = VertexToVertex(g_vertices_dynamic[indices[2] + baseVertex +instanceData.instanceIndexInModel * modelData.vertexCount]);
	}
	else
	{
		vertex0 = VertexToVertex(g_vertices[indices[0] + baseVertex]);
		vertex1 = VertexToVertex(g_vertices[indices[1] + baseVertex]);
		vertex2 = VertexToVertex(g_vertices[indices[2] + baseVertex]);

	}

	float3x3 TBNs[3] =
	{
		float3x3(vertex0.tangentU, vertex0.binormV, vertex0.normal),
		float3x3(vertex1.tangentU, vertex1.binormV, vertex1.normal),
		float3x3(vertex2.tangentU, vertex2.binormV, vertex2.normal)
	};

	float2 textureCoods[3] =
	{
		float2(vertex0.texC),
		float2(vertex1.texC),
		float2(vertex2.texC)
	};


	/*float3x3 TBN = HitAttribute(TBNs, attr);
	float3x3 nodeTransform = (float3x3)nodeData.globalTransform;
	float3x3 objectToWorld = (float3x3)ObjectToWorld4x3();
	float3x3 transformMatrix = mul(nodeTransform, objectToWorld);*/

	float3x3 TBN = HitAttribute(TBNs, attr);
	//float3x3 nodeTransform = (float3x3)nodeData.globalTransform;
	float3x3 objectToWorld = (float3x3)ObjectToWorld4x3();
	float3x3 transformMatrix = objectToWorld;

	TBN = mul(TBN, transformMatrix);

	float2 textureCoord = HitAttribute(textureCoods, attr);

	float mipmapLevel = payload.mipmapLevel;

	float3 albedo = SampleTextureByFloat(Textures[instanceData.matIndex0], textureCoord.xy, mipmapLevel);
	albedo = UnGammaCorrection(albedo);

	float roughness = SampleTextureByFloat(Textures[instanceData.matIndex0 + 1], textureCoord.xy, mipmapLevel).x;
	float metallic = SampleTextureByFloat(Textures[instanceData.matIndex0 + 2], textureCoord.xy, mipmapLevel).x;
	float ao = SampleTextureByFloat(Textures[instanceData.matIndex0 + 3], textureCoord.xy, mipmapLevel).x;
	float3 N = SampleTextureByFloat(Textures[instanceData.matIndex0 + 4], textureCoord.xy, mipmapLevel);

	N = normalize(2.0 * N - 1.0);
	N = normalize(mul(N, TBN));

	//Outgoing light direction(���� �����̽� �ȼ����� �������� ����)
	float3 Lo = normalize(scene.eyePosW - hitPosition);

	//ǥ���� Normal�� Outgoing light�� ����
	float cosLo = max(0.0, dot(N, Lo));

	//Specular reflection vector(���� �뵵�� �� �𸣰�����, Normal�� Lo�� ���ؼ� ����ŧ���� ���� ���� ������ ���� ���͸� ���� �� ����
	float3 Lr = 2.0 * cosLo * N - Lo;

	//Frensnel reflentance at normal incidence(for metals use albedo color)
	//�븻�� ������ ���� �Ի��� �� ������ ������ �ݻ�
	float3 Fdielectric = float3(0.04, 0.04, 0.04);//����ü
	float3 F0 = lerp(Fdielectric, albedo, metallic);


	int lightIndex = 0;

	float3 directLighting = 0.0f;
	
	for (int i = 0; i < scene.dirLightCnt; i++, lightIndex++)
	{
		directLighting +=  DirLightCalculation(gLights[lightIndex].direction,
			gLights[lightIndex].strength, Lo, N, cosLo, F0, albedo, metallic, roughness, 1.0f);
	}

	float3 pointLighting = 0.0f;
	for (uint i = 0; i < scene.pointLightCnt; i++, lightIndex++)
	{
		pointLighting += PointLightCalculation(gLights[lightIndex].position,
			gLights[lightIndex].strength, gLights[lightIndex].falloffStart,
			gLights[lightIndex].falloffEnd, hitPosition,
			Lo, N, cosLo, F0, albedo, metallic, roughness);
	}

	float3 spotLighting = 0.0f;

	for (uint i = 0; i < scene.spotLightCnt; i++, lightIndex++)
	{
		spotLighting += SpotLightCalculation(gLights[lightIndex].position,
			gLights[lightIndex].direction, gLights[lightIndex].strength,
			gLights[lightIndex].falloffStart, gLights[lightIndex].falloffEnd,
			gLights[lightIndex].spotPower, hitPosition,
			Lo, N, cosLo, F0, albedo, metallic, roughness);
	}



	//float3 directLighting = 0.0;
	//for (uint i = 0; i < scene.dirLightCnt; i++)
	//{
	//	float3 Li = -gLights[i].direction;
	//	float3 Lradiance = gLights[i].strength;

	//	//Li�� Lo������ ��������
	//	float3 Lh = normalize(Li + Lo);

	//	//Normal�� �پ��� �� ���� ������ ���� ����Ѵ�.
	//	float cosLi = max(0.0, dot(N, Li));
	//	float cosLh = max(0.0, dot(N, Lh));

	//	//direct light������ fresnel ���
	//	float3 F = FresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
	//	//Specular BRDF������ normal ������ ���
	//	float D = NDFGGX(cosLh, roughness);
	//	// Calculate geometric attenuation for specular BRDF.
	//	float G = GASchlickGGX(cosLi, cosLo, roughness);

	//	// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
	//	// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
	//	// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
	//	float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);

	//	// Lambert diffuse BRDF.
	//	// We don't scale by 1/PI for lighting & material units to be more convenient.
	//	// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	//	float3 diffuseBRDF = kd * albedo;

	//	// Cook-Torrance specular microfacet BRDF.
	//	float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

	//	// Total contribution for this light.
	//	directLighting += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	//}

	Ray ray;
	ray.origin = hitPosition;

	//float3 eyeToRayOrigin = -(gEyePosW - hitPosition);
	//eyeToRayOrigin = normalize(eyeToRayOrigin);
	//ray.direction = reflect(eyeToRayOrigin, N);

	ray.direction = reflect(WorldRayDirection(), N);

	float4 reflectedColor = TraceRadianceRay(ray, payload.recursionDepth, roughness);

	float3 ambientLighting;

	float3 indirectLighting = 0;
	{
		float3 irradiance = float3(reflectedColor.xyz);

		float3 F = FresnelSchlick(F0, cosLo);

		float3 kd = lerp(1.0 - F, 0.0, metallic);

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
		ambientLighting = diffuseIBL + specularIBL;


	}

	//float3 Ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao;


	//float3 color = directLighting + indirectLighting+ Ambient;
	float3 color = directLighting + pointLighting+ spotLighting+  ambientLighting* ao;


	//float Gamma = 1.0f / 2.2f;
	//color = color / (color + float3(1.0f, 1.0f, 1.0f));
	//color = pow(color, float3(Gamma, Gamma, Gamma));


	payload.color = float4(color, 1.0f);
	//payload.recursionDepth = 0;
}




[shader("miss")]
void Miss(inout Payload payload : SV_Payload)
{
	float mipmapLevel = payload.mipmapLevel;

	


	float3 skyboxColor = Skybox.SampleLevel(gsamLinearWrap, WorldRayDirection(), mipmapLevel);

	payload.color = float4(UnGammaCorrection(skyboxColor),1);
	//payload.recursionDepth = 0;

}