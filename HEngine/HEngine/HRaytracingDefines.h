#pragma once

using namespace DX;
using namespace Microsoft::WRL;
using namespace DirectX;

//RayGeneration은 상관없지만 Hit 와 Miss SBT의 경우
//크기가 256의 배수로 고정되어야 한다.
struct __declspec(align(256)) ShaderParameter
{
	char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
	UINT64 RootParameters[7]; //사용하지 않음
};
//
//namespace
//{
//	inline DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt)
//	{
//		switch (fmt)
//		{
//		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
//		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
//		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
//		default:                                return fmt;
//		}
//	}
//};



enum class SRVUAVDescriptorHeapOffset_Raytracing
{
	OutputBuffer = 0,
	Material = OutputBuffer + 1 ,
	Skybox = Material + 350,
	ColorChip = Skybox + 1,
	Normal = ColorChip + 4,
	DepthMap,
	MetallicRoughnessAoEmissive,
	RandomTexture,
	TotalCount

};

enum class NonShaderVisibleDescriptorHeap : size_t
{
	eOutputBuffer = 0,
	TotalCount
};

enum class GlobalRootSig
{
	eSceneConstant,
	eSceneAS,
	TotalCount,
};

enum class LocalRootSig_Raygen
{
	eOutputBuffer,					//heap range : register(u0)
	eNormalBuffer,					//heap range : register(t0, space1)
	eDepthBuffer,					//heap range : register(t1, space1)
	eMetallicRoughnessAoEmissive,   //heap range : register(t3, space1)
	eRandomTexture,					//heap range : register(t4, space1)
	eTotalCount
};

enum class LocalRootSig_Miss
{
	eSkyBox,		//root parameter : register(t0, space1)
	eTotalCount
};

enum class LocalRootSig_Hit0
{
	eStaticVertexBuffer,  //root parameter : register(t4, space1)
	eDynamicVertexBuffer, //root parameter : register(t0, space1)
	eIndexBuffer,		  //root parameter : register(t3, space1)
	eLight,				  //root parameter : register(t2, space1)
	eInstanceData,		  //root parameter : register(t1, space1)
	eModelData,			  //root parameter : register(t5, space1)
	eMaterialTextures,	  //heap range	   : register(t0, space2)
	eTotalCount
};

enum class LocalRootSig_Hit1
{
	eStaticVertexBuffer,  //root parameter : register(t4, space1)
	eDynamicVertexBuffer, //root parameter : register(t0, space1)
	eIndexBuffer,		  //root parameter : register(t3, space1)
	eLight,				  //root parameter : register(t2, space1)
	eInstanceData,		  //root parameter : register(t1, space1)
	eModelData,			  //root parameter : register(t5, space1)
	eColorChipTexture,    //heap range	   : register(t0, space2)
	eTotalCount
};

