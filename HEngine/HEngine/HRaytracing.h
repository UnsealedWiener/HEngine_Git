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
	GraphicsMemory* m_pGraphicsMemory;

	//raytracing resources
	std::unique_ptr<DescriptorHeap>		m_pDescriptorHeap_SRVUAV;

	//state object
	ComPtr<ID3D12StateObject>			m_pRaytracingStateObject;
	ComPtr<ID3D12StateObjectProperties> m_pRaytracingStateObjectProperties;
	ComPtr<ID3D12RootSignature>			m_pGlobalRootSignature;
	
	ComPtr<ID3D12RootSignature>			m_pLocalRootSig_Raygen;
	ComPtr<ID3D12RootSignature>			m_pLocalRootSig_Miss;
	ComPtr<ID3D12RootSignature>			m_pLocalRootSig_HitGroup0;
	ComPtr<ID3D12RootSignature>			m_pLocalRootSig_HitGroup1;


	//shader binding tables
	ComPtr<ID3D12Resource>				m_pRaygenerationSBT;
	SharedGraphicsResource				m_pHitSBT;
	SharedGraphicsResource				m_pMissSBT;



public:
	void Initialize(DeviceResources* pDeviceResources, GraphicsMemory* pGraphicsMemory, HModelManager* pModelManager, HPassConstant* pPassConstant, HTextureManager* pTextureManager);
	void CreateDeviceDependentResources(ResourceUploadBatch& resourceBatch);
	//void CreateWindowSizeDepedentResources();
	void CreateDescriptors(ID3D12Resource* pReflection, ID3D12Resource* pNormal, ID3D12Resource* pDepth, ID3D12Resource * pMetallicRoughnessAoEmissive, ID3D12Resource* pRandomTexture);
	void Update();
	void Draw();
	void ClearBuffer(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
private:
	
	void CreateGlobalRootSignature();
	void CreateLocalRootSignature_Raygen();
	void CreateLocalRootSignature_Miss();
	void CreateLocalRootSignature_HitGroup0();
	void CreateLocalRootSignature_HitGroup1();



	void CreateRaytracingStateObject();
	void InitializeShaderBindingTable(); 
	void UpdateShaderBindingTable();


};