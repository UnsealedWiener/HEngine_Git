#pragma once

using namespace DX;
using namespace Microsoft::WRL;
using namespace DirectX;

//RayGeneration�� ��������� Hit �� Miss SBT�� ���
//ũ�Ⱑ 256�� ����� �����Ǿ�� �Ѵ�.
struct __declspec(align(256)) ShaderParameter
{
	char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
	UINT64 RootParameters[1]; //������� ����
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

enum class RootSig_Raytracing
{
	OutputBuffer,
	RaytracingAccelerationStructure,
	PassConstant,
	IndexBuffer,
	VertexBuffer,
	VertexBuffer_Dynamic,
	structuredBuffer_PerInstance,
	structuredBuffer_PerModel,
	Material,
	GBuffer,
	Light,
	MaxCount,
};

enum class SRVUAVDescriptorHeapOffset_Raytracing
{
	OutputBuffer = 0,
	Material = 1,
	Skybox = 301,
	Normal,
	DepthMap,
	MaxCount

};

enum class NonShaderVisibleDescriptorHeap : size_t
{
	eOutputBuffer = 0,
	TotalCount
};

