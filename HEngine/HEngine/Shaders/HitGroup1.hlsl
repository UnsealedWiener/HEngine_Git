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

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
//지정하지 않은 경우 DirectX Raytracing API가 자동으로 생성하는
//히트된 삼각형의 특성을 정의합니다.
struct Build_in_attribute
{
	float BaryX;
	float BaryY;
};

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

StructuredBuffer<Light> gLights : register(t2, space1);

ByteAddressBuffer g_indices : register(t3, space1);
StructuredBuffer<Vertex_Static> g_vertices : register(t4, space1);
StructuredBuffer<Vertex_Dynamic> g_vertices_dynamic : register(t0, space1);

StructuredBuffer<RaytracingDataPerInstance> g_instanceData : register(t1, space1);
StructuredBuffer<RaytracingDataPerModel> g_modelsData : register(t5, space1);

// An array of textures, which is only supported in shader model 5.1+.  Unlike Texture2DArray, the textures
// in this array can be different sizes and formats, making it more flexible than texture arrays.
//MatIndex + 0 = albedo
//MatIndex + 1 = roughness
//MatIndex + 2 = metallic
//MatIndex + 3 = ao
//MatIndex + 4 = normal
//MatIndex + 5 = height

Texture2D baseColorTexture:    register(t0, space2);
Texture2D roughnessTexture:    register(t1, space2);
Texture2D metallicTexture:    register(t2, space2);
Texture2D emissiveTexture: register(t3, space2);



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

float3 SampleTextureByFloat(Texture2D inputTexture, float2 texCoord, float value)
{
	uint width;
	uint height;
	uint mipmapNum;

	inputTexture.GetDimensions(0, width, height, mipmapNum);

	return inputTexture.SampleLevel(gsamLinearWrap, texCoord.xy, mipmapNum * value).xyz;
}

[shader("closesthit")]
void Hit1(inout Payload payload : SV_Payload, in MyAttributes attr)
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

	float3 albedo = SampleTextureByFloat(baseColorTexture, textureCoord.xy, mipmapLevel);
	albedo = UnGammaCorrection(albedo);

	float roughness = SampleTextureByFloat(roughnessTexture, textureCoord.xy, 0).x;
	float metallic = SampleTextureByFloat(metallicTexture, textureCoord.xy, 0).x;
	float emissive = SampleTextureByFloat(emissiveTexture, textureCoord.xy, 0).x;
	float3 N = TBN[2];
	N = normalize(N);
	//Outgoing light direction(월드 스페이스 픽셀에서 눈으로의 벡터)
	float3 Lo = normalize(scene.eyePosW - hitPosition);

	//표면의 Normal과 Outgoing light의 각도
	float cosLo = max(0.0, dot(N, Lo));

	//Specular reflection vector(아직 용도는 잘 모르겠지만, Normal과 Lo를 통해서 스펙큘러가 가장 높게 나오는 빛의 벡터를 구한 것 같다
	float3 Lr = 2.0 * cosLo * N - Lo;

	//Frensnel reflentance at normal incidence(for metals use albedo color)
	//노말과 동일한 빛이 입사할 때 나오는 프레넬 반사
	float3 Fdielectric = float3(0.04, 0.04, 0.04);//유전체
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

	Ray ray;
	ray.origin = hitPosition;

	ray.direction = reflect(WorldRayDirection(), N);


	float3 reflectedColor = TraceRadianceRay(ray, payload.recursionDepth, roughness).xyz;
	
	
	
	/*ray.direction.x -= 0.7;
	ray.direction = normalize(ray.direction);
	float3 reflectedColor1 = TraceRadianceRay(ray, payload.recursionDepth, roughness).xyz;
	ray.direction.x -= 0.7;
	ray.direction = normalize(ray.direction);
	float3 reflectedColor2 = TraceRadianceRay(ray, payload.recursionDepth, roughness).xyz;

	reflectedColor = (0);*/

	float3 ambientLighting = 0;
	{
		float3 irradiance = reflectedColor;

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

	float3 color = directLighting + pointLighting+ spotLighting+  ambientLighting* 0.5/*ao*/;
	color += albedo* emissive * 10;
	payload.color = float4(color, 1.0f);
}

