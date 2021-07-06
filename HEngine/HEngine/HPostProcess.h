#pragma once
#include"HPostProcessDefines.h"
#include"d3dUtil.h"

class HPassConstant;

namespace DX
{
	class DeviceResources;
}

class HPostProcess
{
private:
	HPostProcess() {};
private:
	const DeviceResources* m_pDeviceResources;
	const HPassConstant* m_pPassConstant;

	std::unique_ptr<DescriptorHeap> m_pDesciptorHeap_SRVUAV_DOF;
	ComPtr<ID3D12RootSignature>		m_pComputeRootSig_DOF;
	ComPtr<ID3D12PipelineState>	    m_pComputePSO_DOF;

	std::unique_ptr<DescriptorHeap> m_pDesciptorHeap_SRVUAV_verticalBlur;
	std::unique_ptr<DescriptorHeap> m_pDesciptorHeap_SRVUAV_horizonBlur;
	ComPtr<ID3D12PipelineState>	    m_pComputePSO_verticalBlur;
	ComPtr<ID3D12PipelineState>	    m_pComputePSO_horizonBlur;

	ComPtr<ID3D12RootSignature>		m_pComputeRootSig_blurCommon;

	DOFParam m_DOFParam;
	SharedGraphicsResource m_DOFParamConstantBuffer;


public:
	static HPostProcess* GetInstance();

	void Initialize(DeviceResources* pDeviceResources, HPassConstant* pPassConstant);
	void CreateDeviceDependentResources();
	void CreateDescriptors(ID3D12Resource* pDepth,
		ID3D12Resource* pSceneBuffer, ID3D12Resource* pFinalResultBuffer,
		ID3D12Resource* pVerticalBufferBuffer, ID3D12Resource* pHorizonBufferBuffer);
	void DrawDOF();
	void DrawVerticalBlur();
	void DrawHorizonBlur();
	void SetDOFParams(float focusDepth, float maxBlurDepthGap);

private:


	void CreateDescriptorHeap_DOF();
	void CreateRootSignature_DOF();
	void CreateComputePSO_DOF();
	void CrateDescriptor_DOF(ID3D12Resource* pDepth,
		ID3D12Resource* pSceneBuffer,
		ID3D12Resource* pFinalResultBuffer,
		ID3D12Resource* pHorizonBufferBuffer);

	void CreateDescriptorHeap_Blurs();
	void CreateDescriptors_Blurs(ID3D12Resource* pSceneBuffer,
		ID3D12Resource* pVerticalBufferBuffer,
		ID3D12Resource* pHorizonBufferBuffer);
	void CreateRootSignature_Blurs();
	void CreateComputePSO_Blurs();
	
};

