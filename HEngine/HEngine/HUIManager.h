#pragma once
#include"HUIManagerDefines.h"

namespace DX
{
	class DeviceResources;
}
class HTextureManager;

using namespace DX;
using namespace Microsoft::WRL;

class HUIManager
{
	const DeviceResources* m_pDeviceResources;
	const HTextureManager* m_pTextureManager;
	GraphicsMemory* m_pGraphicMemory;

	std::unordered_map<void*, std::unique_ptr<HUI>> m_uiList;


	ComPtr<ID3D12RootSignature>										m_pRootSignature;
	ComPtr<ID3D12PipelineState>										m_PSO;

	ComPtr<ID3D12Resource>											m_verticesResource;
	ComPtr<ID3D12Resource>											m_indicesResource;

	SharedGraphicsResource											m_structuredBuffer_perInstance;

public:
	static HUIManager* GetInstance();
	void Initialize(GraphicsMemory* pGraphicMemory, DeviceResources* pDeviceResources, HTextureManager* pTextureManager);
	void Draw();
	void Update();
	void CreateDeviceDependentResource();
	HUIData* CreateUI();

private:
	void CreateRootSignature_lightCalculation();
	void CreatePSO();
	void CreateVertexIndexBuffer();
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();

	HUIManager(){}

};
