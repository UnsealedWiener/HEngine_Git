#pragma once

using namespace DX;
using namespace Microsoft::WRL;
using namespace DirectX;

namespace HRasterizeDefines
{
	enum class PSOTYPE
	{

		DEFAULT,
		COLORCHIP,
		WIREFRAME
	};

	PSOTYPE operator*(PSOTYPE c);
	PSOTYPE operator++(PSOTYPE& x);
	PSOTYPE begin(PSOTYPE r);
	PSOTYPE end(PSOTYPE r);




	enum class RootSig_Rasterize
	{
		StructuredBuffer_instances,
		StructuredBuffer_instancesBones,
		StructuredBuffer_perPSO,
		ConstantBuffer_perModel,
		ConstantBuffer_pass,
		Textures,
		ColorChip,
		TotalCount
	};

	enum class SRVUAVDescriptorHeapOffset_Rasterize
	{
		Material = 0,//material 350
		SkyBox = Material + 350,//skybox 1
		ColorChip = SkyBox + 1,//colorchip 4
		TotalCount = ColorChip + 4,
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
		eEmissive,
		TotalCount,
	};

	enum class DescriptorHeapList_DSV
	{
		eShadow,
		TotalCount,
	};

}