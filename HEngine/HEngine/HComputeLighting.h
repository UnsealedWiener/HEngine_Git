#pragma once
#include"EngineInterface.h"
#include"HComputeLightingDefines.h"
#include"d3dUtil.h"

namespace DX
{
	class DeviceResources;
}

class HPassConstant;

class HComputeLighting
{
private:
	const DeviceResources*			m_pDeviceResources;
	const HPassConstant*			m_pPassConstant;
	
	ComPtr<ID3D12RootSignature>		m_pComputeRootSig_lightCalculation;
	ComPtr<ID3D12PipelineState>	    m_pComputePso_lightCalculation;
	std::unique_ptr<DescriptorHeap> m_pDesciptorHeap_SRVUAV_lightCalculation;


	std::unique_ptr<DescriptorHeap> m_pDesciptorHeap_SRVUAV_ssao;
	ComPtr<ID3D12RootSignature>		m_pComputeRootSig_ssao;
	ComPtr<ID3D12PipelineState>	    m_pComputePso_ssao;


	


public:
	void Initialize(DeviceResources* pDeviceResources, HPassConstant* pPassConstant);
	void CreateDeviceDependentResources();

	void CreateDescriptors (ID3D12Resource* pAlbedo, ID3D12Resource* pMetallicRoughnessAo,
		ID3D12Resource* pNormal, ID3D12Resource* pEmissive, ID3D12Resource* pDepth, ID3D12Resource* pReflection, ID3D12Resource* pShadow,
		ID3D12Resource* pRandomVector, ID3D12Resource* pSSAO,
		ID3D12Resource* pScene);



	void ComputeLighting();
	void DrawSSAO();
	void ClearSSAO(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

private:
	void CreateDescriptorHeap();
	void CreateComputePSO_lightCalculation();
	void CreateRootSignature_lightCalculation();
	void CreateRootSignature_ssao();
	void CreateComputePSO_ssao();


	void CreateDesciptors_lightCalculation(ID3D12Resource* pAlbedo, ID3D12Resource* pMetallicRoughnessAo,
		ID3D12Resource* pNormal, ID3D12Resource* pEmissive, ID3D12Resource* pDepth, ID3D12Resource* pReflection, ID3D12Resource* pShadow,
		ID3D12Resource* pSSAO, ID3D12Resource* pResult);

	void CreateDesciptors_ssao(ID3D12Resource* pDepth, ID3D12Resource* pNormal, ID3D12Resource* pRandomVector,
		ID3D12Resource* pSSAO);
	

};
