#include "pch.h"
#include "HIrradianceMapGenerator.h"

HIrradianceMapGenerator* HIrradianceMapGenerator::GetInstance()
{
	HIrradianceMapGenerator generator;

	return &generator;
}

void HIrradianceMapGenerator::GenerateIrradianceMapFromCubeMap(
	Microsoft::WRL::ComPtr<ID3D12Resource>& resultIrradianceMap,
	ID3D12Device* pDevice,
	DirectX::ResourceUploadBatch& batch,
	ID3D12Resource* pOriginCubeMap)
{
	D3D12_RESOURCE_DESC cubeMapResourceDesc = pOriginCubeMap->GetDesc();
	cubeMapResourceDesc.MipLevels = 1;
	cubeMapResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	pDevice->CreateCommittedResource(
		&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&cubeMapResourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_GRAPHICS_PPV_ARGS(&resultIrradianceMap));



}
