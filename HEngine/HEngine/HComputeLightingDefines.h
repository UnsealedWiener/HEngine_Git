#pragma once

using namespace DX;
using namespace Microsoft::WRL;
using namespace DirectX;

namespace HComputeLightingDefines
{
	enum class DescriptorHeapList_SRVUAV_lightCalculation
	{
		eAlbedo,
		eMetallicRoughnessAo,
		eNormal,
		eDepth,
		eReflect,
		eShadow,
		eSSAO,
		eOutput,

		InputTextureStart = eAlbedo,
		OutputTexutreStart = eOutput,

		OutputTextureCount = 1,
		InputTextureCount = OutputTexutreStart - InputTextureStart,

		TotalCount = InputTextureCount + OutputTextureCount
	};

	enum class DescriptorHeapList_SRVUAV_ssao
	{
		eDepth,
		eNormal,
		eRandomVector,
		eOutput,

		InputTextureStart = eDepth,
		OutputTexutreStart = eOutput,

		OutputTextureCount = 1,
		InputTextureCount = OutputTexutreStart - InputTextureStart,

		TotalCount = InputTextureCount + OutputTextureCount
	};

	enum class RootSignatureList_lightCalculation
	{
		eTexuresInput,
		eTextureOutput,
		ePassConstant,
		eLight,
		TotalCount
	};

	enum class RootSignatureList_ssao
	{
		eTexuresInput,
		eTextureOutput,
		ePassConstant,
		TotalCount
	};


}
