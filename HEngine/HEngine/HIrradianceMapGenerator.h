#pragma once
class HIrradianceMapGenerator
{

public:
	static HIrradianceMapGenerator* GetInstance();


	void GenerateIrradianceMapFromCubeMap(
		Microsoft::WRL::ComPtr<ID3D12Resource>& resultIrradianceMap,
		ID3D12Device* pDevice,
		DirectX::ResourceUploadBatch& batch,
		ID3D12Resource* pOriginCubeMap);

};

