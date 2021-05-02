#pragma once
#include"HRaytracingDefines.h"



namespace DX
{
	class DeviceResources;
}
class HModelManager;
class HPassConstant;
class HTextureManager;


class HRaytracing
{
	const DeviceResources*	m_pDeviceResources;
	HModelManager*	m_pModelManager;
	const HPassConstant*	m_pPassConstant;
	const HTextureManager*	m_pTextureManager;

	//raytracing resources
	std::unique_ptr<DescriptorHeap>		m_pDescriptorHeap_SRVUAV;

	//state object
	ComPtr<ID3D12StateObject>			m_pRaytracingStateObject;
	ComPtr<ID3D12StateObjectProperties> m_pRaytracingStateObjectProperties;
	ComPtr<ID3D12RootSignature>			m_pRaytracingGlobalRootSignature;
	
	//shader binding tables
	ComPtr<ID3D12Resource>				m_pRaygenerationSBT;
	ComPtr<ID3D12Resource>				m_pHitSBT;
	ComPtr<ID3D12Resource>				m_pMissSBT;



public:
	void Initialize(DeviceResources* pDeviceResources, HModelManager* pModelManager, HPassConstant* pPassConstant, HTextureManager* pTextureManager);
	void CreateDeviceDependentResources(ResourceUploadBatch& resourceBatch);
	//void CreateWindowSizeDepedentResources();
	void CreateDescriptors(ID3D12Resource* pReflection, ID3D12Resource* pNormal, ID3D12Resource* pDepth);
	void Update();
	void Draw();
	void ClearBuffer(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
private:
	
	void CreateRaytracingRootSignature();
	void CreateRaytracingStateObject();
	void CreateShaderBindingTable(ResourceUploadBatch& resourceBatch);


};