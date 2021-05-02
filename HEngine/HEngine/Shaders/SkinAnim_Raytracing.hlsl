

struct VertexIn
{
	float3 pos;
	float3 normal;
	float3 tangentU;
	float3 biNormV;
	float2 texC;
	uint4 boneIndex;
	float4 blendWeight;
};

struct VertexOut
{
	float3 pos;
	float3 normal;
	float3 tangentU;
	float3 biNormV;
	float2 texC;
	int	   padding[2];
};

cbuffer ModelData : register(b0)
{
	uint g_instanceCount;
	uint g_vertexCount;
	uint g_boneCount;
	uint g_vertexRecordOffset;

};

//원본 버텍스 예시
//예시 : 모델1 
//버텍스 구성 : 모델1

StructuredBuffer<VertexIn> originalVertex : register(t0);

//bone TM 구성 
//예시 : 
//TM 구성 : 모델1(인스턴스1(TM1,TM2,TM3), 인스턴스2(TM1,TM2,TM3)) 

StructuredBuffer<float4x4> boneTM : register(t1, space1);

//출력 버텍스 예시
//예시 : 모델1(본O) + 모델2(본X) + 모델3(본O)
//버텍스 구성 : 모델1(인스턴스1, 인스턴스2, 인스턴스3) + 모델2(원본 버텍스) + 모델3(인스턴스1, 인스턴스2)
RWStructuredBuffer<VertexOut> resultVertex : register(u0);

float4x4 Skinning(uint4 boneIndex, float4 blendWeight, uint boneIndexOffset)
{
	float4x4 mat0 = boneTM[boneIndexOffset + boneIndex.x];
	float4x4 mat1 = boneTM[boneIndexOffset + boneIndex.y];
	float4x4 mat2 = boneTM[boneIndexOffset + boneIndex.z];
	float4x4 mat3 = boneTM[boneIndexOffset + boneIndex.w];

	float4x4 result =
		mat0 * blendWeight.x +
		mat1 * blendWeight.y +
		mat2 * blendWeight.z +
		mat3 * blendWeight.w;




	return result;
}

[numthreads(256, 1, 1)]
void CS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	uint originVertexAddress = dispatchThreadID.x;
	uint instanceIndex = dispatchThreadID.y;

	if (originVertexAddress < g_vertexCount)
	{
		uint vertexRecordAddress = g_vertexRecordOffset + g_vertexCount * instanceIndex + originVertexAddress;

		VertexIn originVertex = originalVertex[originVertexAddress];

		uint boneTMsOffset = instanceIndex * g_boneCount;

		float4x4 skinnedTM = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

		if (dot(originVertex.blendWeight, float4(1, 1, 1, 1) != 0))
		{
			skinnedTM = Skinning(originVertex.boneIndex, originVertex.blendWeight, boneTMsOffset);
			//skinnedTM = Skinning(originVertex.boneIndex, originVertex.blendWeight, 0);

		}
		//VertexOut skinnedVertex;

		//float4x4 test = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 2, 0, 0, 1);

		//resultVertex[vertexRecordAddress].pos = mul(float4(originVertex.pos, 1.0f), test).xyz;

		//float4x4 mat0 = (boneTM[0]);
		//float3 pos = 0;
		//pos += mul( mat0, float4(originVertex.pos, 1.0f)).xyz;
		//pos += mul(float4(originVertex.pos, 1.0f), mat0 ).xyz;


		//resultVertex[vertexRecordAddress].pos = pos;

		resultVertex[vertexRecordAddress].pos = mul(float4(originVertex.pos, 1.0f), (float4x4)skinnedTM).xyz;
		resultVertex[vertexRecordAddress].normal = mul(originVertex.normal, (float3x3)skinnedTM).xyz;
		resultVertex[vertexRecordAddress].tangentU = mul(originVertex.tangentU, (float3x3)skinnedTM).xyz;
		resultVertex[vertexRecordAddress].biNormV = mul(originVertex.biNormV, (float3x3)skinnedTM).xyz;
		resultVertex[vertexRecordAddress].texC = originVertex.texC;

		//resultVertex[vertexRecordAddress].pos = originVertex.pos;
		//resultVertex[vertexRecordAddress].normal = originVertex.normal;
		//resultVertex[vertexRecordAddress].tangentU = originVertex.tangentU;
		//resultVertex[vertexRecordAddress].biNormV = originVertex.biNormV;
		//resultVertex[vertexRecordAddress].texC = originVertex.texC;





	}
}
