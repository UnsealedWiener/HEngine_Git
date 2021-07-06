#pragma once
#include"HRasterizeDefines.h"
#include"EngineInterface.h"
#include"HSimplePrimitiveManager.h"

namespace DX
{
	class DeviceResources;
}
class HModelManager;
class HPassConstant;
class HTextureManager;
struct HModel;
struct HMesh;

class HRasterize
{
	const DeviceResources*			m_pDeviceResources;
	const HModelManager*			m_pModelManager;
	const HPassConstant*			m_pPassConstant;
	const HTextureManager*			m_pTextureManager;

	ComPtr<ID3D12RootSignature>										m_pRootSignature;
	std::unordered_map<HRasterizeDefines::PSOTYPE, ComPtr<ID3D12PipelineState>>		m_PSO_default;
	ComPtr<ID3D12PipelineState>										m_shadowPSO;

	std::unique_ptr<DescriptorHeap> m_pDescriptorHeap_SRV;
	std::unique_ptr<DescriptorHeap> m_pDescriptorHeap_RTV;
	std::unique_ptr<DescriptorHeap> m_pDescriptorHeap_DSV;

public:

	void Initialize(DeviceResources* pDeviceResources, HModelManager* pModelManager, HPassConstant* pPassConstant, HTextureManager* pTextureManager, Camera* m_pCamera);
	//void Draw(bool bMsaa, bool wireframe);
	void DrawGBuffer(bool wireframe);
	void DrawShadow();
	void ClearShadowBuffer();
	void Update();
	void CreateDeviceDependentResources();
	void CreateDescriptors (ID3D12Resource* pAlbedo, ID3D12Resource* pMetallicRoughnessAo, ID3D12Resource* pEmissive,
		ID3D12Resource* pNormal, ID3D12Resource* pEdge);

private:

	void CreateDescriptorHeap();
	void CreateRootSignature();
	void CreatePSOs();
	void CreateShadowBufferPSO(D3D12_SHADER_BYTECODE VS, DXGI_SAMPLE_DESC sampleDesc,
		D3D12_RASTERIZER_DESC rasterDesc,
		ID3D12PipelineState** outputPSO);
	void CreatePSO(D3D12_SHADER_BYTECODE VS, D3D12_SHADER_BYTECODE PS, D3D12_SHADER_BYTECODE HS, D3D12_SHADER_BYTECODE DS,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType, DXGI_SAMPLE_DESC sampleDesc,
		D3D12_RASTERIZER_DESC rasterDesc, ID3D12PipelineState** outputPSO);

};