#include "pch.h"
#include "HPostProcess.h"
#include "HPassConstant.h"
#include"DeviceResources.h"
#include"DOF_CS.hlsl.h"
#include"horizonBlur_CS.hlsl.h"
#include"verticalBlur_CS.hlsl.h"

HPostProcess* HPostProcess::GetInstance()
{
	static HPostProcess postProcess;
	return &postProcess;
}

void HPostProcess::Initialize(
	DeviceResources* pDeviceResources,
	HPassConstant* pPassConstant)
{
	m_pDeviceResources = pDeviceResources;
	m_pPassConstant = pPassConstant;
}

void HPostProcess::CreateDeviceDependentResources()
{
	auto device = m_pDeviceResources->GetD3DDevice();
	CreateDescriptorHeap_DOF();
	CreateDescriptorHeap_Blurs();
	CreateRootSignature_DOF();
	CreateRootSignature_Blurs();
	CreateComputePSO_DOF(); 
	CreateComputePSO_Blurs();
}

void HPostProcess::CreateDescriptorHeap_DOF()
{
	auto device = m_pDeviceResources->GetD3DDevice();
	m_pDesciptorHeap_SRVUAV_DOF
		= std::make_unique<DescriptorHeap>(device,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			(UINT)DescriptorHeapList_SRVUAV_DOF::eTotalCount);
}


void HPostProcess::CreateDescriptorHeap_Blurs()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	m_pDesciptorHeap_SRVUAV_verticalBlur
		= std::make_unique<DescriptorHeap>(device,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			(UINT)DescriptorHeapList_SRVUAV_blur::eTotalCount);

	m_pDesciptorHeap_SRVUAV_horizonBlur
		= std::make_unique<DescriptorHeap>(device,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			(UINT)DescriptorHeapList_SRVUAV_blur::eTotalCount);
}



void HPostProcess::CreateRootSignature_DOF()
{
	HRESULT hr = S_OK;

	CD3DX12_DESCRIPTOR_RANGE textureInputDescriptorTable;
	textureInputDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		3, 0, 0);//sceneBuffer, depthBuffer, blurredBuffer

	CD3DX12_DESCRIPTOR_RANGE textureOutputDescriptorTable;
	textureOutputDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		1, 0, 0, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)RootSignitureList_DOF::eTotalCount];

	rootParameters[(UINT)RootSignitureList_DOF::eTextureInput].InitAsDescriptorTable(1, &textureInputDescriptorTable, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSignitureList_DOF::eOutputBuffer].InitAsDescriptorTable(1, &textureOutputDescriptorTable, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSignitureList_DOF::ePassConstant].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSignitureList_DOF::eDOFParam].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);


	auto staticSamplers = d3dUtil::GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(ARRAYSIZE(rootParameters), rootParameters,
		(UINT)staticSamplers.size(), staticSamplers.data());


	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	DX::ThrowIfFailed(hr);

	DX::ThrowIfFailed(m_pDeviceResources->GetD3DDevice()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&m_pComputeRootSig_DOF)));
}

void HPostProcess::CreateRootSignature_Blurs()
{
	HRESULT hr = S_OK;

	CD3DX12_DESCRIPTOR_RANGE textureInputDescriptorTable;
	textureInputDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 0, 0);//originBuffer

	CD3DX12_DESCRIPTOR_RANGE textureOutputDescriptorTable;
	textureOutputDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		1, 0, 0, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)RootSignitureList_blur::eTotalCount];

	rootParameters[(UINT)RootSignitureList_blur::ePassConstant].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSignitureList_blur::eOriginBuffer].InitAsDescriptorTable(1, &textureInputDescriptorTable, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSignitureList_blur::eOutputBuffer].InitAsDescriptorTable(1, &textureOutputDescriptorTable, D3D12_SHADER_VISIBILITY_ALL);

	auto staticSamplers = d3dUtil::GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(ARRAYSIZE(rootParameters), rootParameters,
		(UINT)staticSamplers.size(), staticSamplers.data());


	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	DX::ThrowIfFailed(hr);

	DX::ThrowIfFailed(m_pDeviceResources->GetD3DDevice()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&m_pComputeRootSig_blurCommon)));

}

void HPostProcess::CreateComputePSO_Blurs()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {};
	computePipelineStateDesc.pRootSignature = m_pComputeRootSig_blurCommon.Get();
	computePipelineStateDesc.CS =
	{
		g_pverticalBlur_CS,
		sizeof(g_pverticalBlur_CS)
	};
	computePipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	DX::ThrowIfFailed(device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&m_pComputePSO_verticalBlur)));

	computePipelineStateDesc.CS =
	{
		g_phorizonBlur_CS,
		sizeof(g_phorizonBlur_CS)
	};
	DX::ThrowIfFailed(device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&m_pComputePSO_horizonBlur)));
}

void HPostProcess::CreateComputePSO_DOF()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {};
	computePipelineStateDesc.pRootSignature = m_pComputeRootSig_DOF.Get();
	computePipelineStateDesc.CS =
	{
		g_pDOF_CS,
		sizeof(g_pDOF_CS)
	};
	computePipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	DX::ThrowIfFailed(device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&m_pComputePSO_DOF)));

}

void HPostProcess::CrateDescriptor_DOF(
	ID3D12Resource* pDepth,
	ID3D12Resource* pSceneBuffer,
	ID3D12Resource* pFinalResultBuffer,
	ID3D12Resource* pHorizonBufferBuffer)
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;

	device->CreateShaderResourceView(
		pSceneBuffer, &srvDesc,
		m_pDesciptorHeap_SRVUAV_DOF->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_DOF::eSceneBuffer));

	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	device->CreateShaderResourceView(
		pHorizonBufferBuffer, &srvDesc,
		m_pDesciptorHeap_SRVUAV_DOF->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_DOF::eBlurredBuffer));

	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	device->CreateShaderResourceView(
		pDepth, &srvDesc,
		m_pDesciptorHeap_SRVUAV_DOF->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_DOF::eDepthBuffer));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	device->CreateUnorderedAccessView(
		pFinalResultBuffer, nullptr, &uavDesc,
		m_pDesciptorHeap_SRVUAV_DOF->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_DOF::eOutputBuffer));
}

void HPostProcess::CreateDescriptors_Blurs(ID3D12Resource* pSceneBuffer, ID3D12Resource* pVerticalBufferBuffer, ID3D12Resource* pHorizonBufferBuffer)
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;

	//vertical
	device->CreateShaderResourceView(
		pSceneBuffer, &srvDesc,
		m_pDesciptorHeap_SRVUAV_verticalBlur->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_blur::eOriginBuffer));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	device->CreateUnorderedAccessView(
		pVerticalBufferBuffer, nullptr, &uavDesc,
		m_pDesciptorHeap_SRVUAV_verticalBlur->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_blur::eOutputBuffer));

	//horizon
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	device->CreateShaderResourceView(
		pVerticalBufferBuffer, &srvDesc,
		m_pDesciptorHeap_SRVUAV_horizonBlur->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_blur::eOriginBuffer));

	device->CreateUnorderedAccessView(
		pHorizonBufferBuffer, nullptr, &uavDesc,
		m_pDesciptorHeap_SRVUAV_horizonBlur->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_blur::eOutputBuffer));
}

void HPostProcess::CreateDescriptors(
	ID3D12Resource* pDepth,
	ID3D12Resource* pSceneBuffer, 
	ID3D12Resource* pFinalResultBuffer,
	ID3D12Resource* pVerticalBufferBuffer,
	ID3D12Resource* pHorizonBufferBuffer)
{
	CrateDescriptor_DOF(pDepth, pSceneBuffer, pFinalResultBuffer, pHorizonBufferBuffer);
	CreateDescriptors_Blurs(pSceneBuffer, pVerticalBufferBuffer, pHorizonBufferBuffer);
}						
						
void HPostProcess::DrawDOF()
{
	//reset DOFParam
	m_DOFParamConstantBuffer.Reset(GraphicsMemory::Get().AllocateConstant(m_DOFParam));
	
	auto commandList = m_pDeviceResources->GetCommandList();

	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Depth Of Field");

	ID3D12DescriptorHeap* descriptorHeapArray[] = { m_pDesciptorHeap_SRVUAV_DOF->Heap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeapArray), descriptorHeapArray);

	commandList->SetPipelineState(m_pComputePSO_DOF.Get());
	commandList->SetComputeRootSignature(m_pComputeRootSig_DOF.Get());
	commandList->SetComputeRootDescriptorTable((UINT)RootSignitureList_DOF::eTextureInput,
		m_pDesciptorHeap_SRVUAV_DOF->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_DOF::eSceneBuffer));
	commandList->SetComputeRootDescriptorTable((UINT)RootSignitureList_DOF::eOutputBuffer,
		m_pDesciptorHeap_SRVUAV_DOF->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_DOF::eOutputBuffer));
	commandList->SetComputeRootConstantBufferView((UINT)RootSignitureList_DOF::ePassConstant,
		m_pPassConstant->GetPassConstantGpuAddress());
	commandList->SetComputeRootConstantBufferView((UINT)RootSignitureList_DOF::eDOFParam,
		m_DOFParamConstantBuffer.GpuAddress());

	RECT outputSize = m_pDeviceResources->GetOutputSize();

	UINT numGroupX = (UINT)ceilf((outputSize.right - outputSize.left) / 16.0f);
	UINT numGroupY = (UINT)ceilf((outputSize.bottom - outputSize.top) / 16.0f);

	commandList->Dispatch(numGroupX, numGroupY, 1);

	PIXEndEvent(commandList);
}

void HPostProcess::DrawVerticalBlur()
{
	auto commandList = m_pDeviceResources->GetCommandList();

	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Vertical blur");

	ID3D12DescriptorHeap* descriptorHeapArray[] = { m_pDesciptorHeap_SRVUAV_verticalBlur->Heap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeapArray), descriptorHeapArray);

	commandList->SetPipelineState(m_pComputePSO_verticalBlur.Get());
	commandList->SetComputeRootSignature(m_pComputeRootSig_blurCommon.Get());
	commandList->SetComputeRootDescriptorTable((UINT)RootSignitureList_blur::eOriginBuffer,
		m_pDesciptorHeap_SRVUAV_verticalBlur->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_blur::eOriginBuffer));
	commandList->SetComputeRootDescriptorTable((UINT)RootSignitureList_blur::eOutputBuffer,
		m_pDesciptorHeap_SRVUAV_verticalBlur->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_blur::eOutputBuffer));
	commandList->SetComputeRootConstantBufferView((UINT)RootSignitureList_blur::ePassConstant,
		m_pPassConstant->GetPassConstantGpuAddress());

	RECT outputSize = m_pDeviceResources->GetOutputSize();

	//블러 버퍼는 절반 크기로 생성함
	UINT width = outputSize.right - outputSize.left;
	width /= 2;
	UINT height = outputSize.bottom - outputSize.top;
	height /= 2;

	UINT numGroupX = (UINT)width;
	UINT numGroupY = (UINT)ceilf(height / 256.0f);

	commandList->Dispatch(numGroupX, numGroupY, 1);

	PIXEndEvent(commandList);
}

void HPostProcess::DrawHorizonBlur()
{
	auto commandList = m_pDeviceResources->GetCommandList();

	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Horizon blur");

	ID3D12DescriptorHeap* descriptorHeapArray[] = { m_pDesciptorHeap_SRVUAV_horizonBlur->Heap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeapArray), descriptorHeapArray);

	commandList->SetPipelineState(m_pComputePSO_horizonBlur.Get());
	commandList->SetComputeRootSignature(m_pComputeRootSig_blurCommon.Get());
	commandList->SetComputeRootDescriptorTable((UINT)RootSignitureList_blur::eOriginBuffer,
		m_pDesciptorHeap_SRVUAV_horizonBlur->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_blur::eOriginBuffer));
	commandList->SetComputeRootDescriptorTable((UINT)RootSignitureList_blur::eOutputBuffer,
		m_pDesciptorHeap_SRVUAV_horizonBlur->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_blur::eOutputBuffer));
	commandList->SetComputeRootConstantBufferView((UINT)RootSignitureList_blur::ePassConstant,
		m_pPassConstant->GetPassConstantGpuAddress());

	RECT outputSize = m_pDeviceResources->GetOutputSize();

	//블러 버퍼는 절반 크기로 생성함
	UINT width = outputSize.right - outputSize.left;
	width /= 2;
	UINT height = outputSize.bottom - outputSize.top;
	height /= 2;

	UINT numGroupX = (UINT)ceilf(width/256.0f);
	UINT numGroupY = (UINT)(height);

	commandList->Dispatch(numGroupX, numGroupY, 1);

	PIXEndEvent(commandList);
}

void HPostProcess::SetDOFParams(float focusDepth, float maxBlurDepthGap)
{
	m_DOFParam.focusDepth = focusDepth;
	m_DOFParam.maxBlurDepthGap = maxBlurDepthGap;
}

