#pragma once

using namespace DX;
using namespace Microsoft::WRL;
using namespace DirectX;

namespace HRasterizeDefines
{

	enum PSOTYPE
	{
		NOOPTION_PSO = 0B00000000,
		MSAA_PSO = 0B00000001,
		WIREFRAME_PSO = 0B00000010,
		TWOMAT_PSO = 0B00000100,
		TESS_PSO = 0B00001000,
		ALLOPTION_PSO = MSAA_PSO | WIREFRAME_PSO | TWOMAT_PSO | TESS_PSO,
		DEFAULT_PSO = MSAA_PSO,
	};

	enum class RootSig_Rasterize
	{
		StructuredBuffer_instances,
		StructuredBuffer_instancesBones,
		StructuredBuffer_perPSO,
		ConstantBuffer_perModel,
		ConstantBuffer_pass,
		Textures,
		MaxCount
	};

	enum class SRVUAVDescriptorHeapOffset_Rasterize
	{
		Material = 0,
		SkyBox = 300,
		MaxCount = 301,
	};

	//enum class RootSig_Compute_DeferredRender
	//{
	//	TextureBuffer_input,
	//	TextureBuffer_output,
	//	ConstantBuffer_pass,
	//	MaxCount
	//};

	enum class DescriptorHeapList_RTV
	{
		eAlbedo,
		eMetallicRoughnessAo,
		eNormal,
		TotalCount,
	};

	enum class DescriptorHeapList_DSV
	{
		eShadow,
		TotalCount,
	};

}