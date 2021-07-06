#pragma once


using namespace DX;
using namespace Microsoft::WRL;
using namespace DirectX;

enum class DescriptorHeapList_SRVUAV_DOF
{
	eSceneBuffer,
	eDepthBuffer,
	eBlurredBuffer,
	eOutputBuffer,
	eTotalCount,
};

enum class RootSignitureList_DOF
{
	ePassConstant,
	eDOFParam,
	eTextureInput, 
	eOutputBuffer,
	eTotalCount
};

enum class DescriptorHeapList_SRVUAV_blur
{
	eOriginBuffer,
	eOutputBuffer,
	eTotalCount,
};

enum class RootSignitureList_blur
{
	ePassConstant,
	eOriginBuffer,
	eOutputBuffer,
	eTotalCount
};

struct DOFParam
{
	float focusDepth = 0.4;
	float maxBlurDepthGap = 0.1;
	int	  padding[2];
};