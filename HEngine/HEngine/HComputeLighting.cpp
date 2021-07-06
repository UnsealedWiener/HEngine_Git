#include"pch.h"
#include"HComputeLighting.h"
#include"HPassConstant.h"
#include"DeviceResources.h"

#include"deferredRender_CS.hlsl.h"
#include"ssao_CS.hlsl.h"

using namespace HComputeLightingDefines;


void HComputeLighting::Initialize(DeviceResources* pDeviceResources, HPassConstant* pPassConstant)
{
	m_pDeviceResources = pDeviceResources;
	m_pPassConstant = pPassConstant;
}

void HComputeLighting::CreateDeviceDependentResources()
{
	CreateDescriptorHeap();
	CreateRootSignature_lightCalculation();
	CreateRootSignature_ssao();
	CreateComputePSO_lightCalculation();
	CreateComputePSO_ssao();
}

void HComputeLighting::CreateDescriptors(ID3D12Resource* pAlbedo, ID3D12Resource* pMetallicRoughnessAo, ID3D12Resource* pNormal, ID3D12Resource* pEmissive, ID3D12Resource* pDepth, ID3D12Resource* pReflection, ID3D12Resource* pShadow, ID3D12Resource* pRandomVector, ID3D12Resource* pSSAO, ID3D12Resource* pResult)
{
	CreateDesciptors_lightCalculation(pAlbedo, pMetallicRoughnessAo, pNormal,pEmissive, pDepth, pReflection, pShadow, pSSAO, pResult);
	CreateDesciptors_ssao(pDepth, pNormal, pRandomVector, pSSAO);
}

void HComputeLighting::CreateDesciptors_lightCalculation(ID3D12Resource* pAlbedo, ID3D12Resource* pMetallicRoughnessAo, ID3D12Resource* pNormal,
	ID3D12Resource* pEmissive, ID3D12Resource* pDepth, ID3D12Resource* pReflection, ID3D12Resource* pShadow, ID3D12Resource* pSSAO,
	ID3D12Resource* pScene)
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;

	device->CreateShaderResourceView(
		pAlbedo, &srvDesc,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::eAlbedo));

	device->CreateShaderResourceView(
		pMetallicRoughnessAo, &srvDesc,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::eMetallicRoughnessAo));

	device->CreateShaderResourceView(
		pNormal, &srvDesc,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::eNormal));

	device->CreateShaderResourceView(
		pEmissive, &srvDesc,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::eEmissive));

	device->CreateShaderResourceView(
		pReflection, &srvDesc,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::eReflect));

	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	device->CreateShaderResourceView(
		pSSAO, &srvDesc,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::eSSAO));

	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	device->CreateShaderResourceView(
		pDepth, &srvDesc,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::eDepth));

	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	device->CreateShaderResourceView(
		pShadow, &srvDesc,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::eShadow));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	//uavDesc.Texture2D.MipSlice = 1;

	device->CreateUnorderedAccessView(
		pScene, nullptr, &uavDesc,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::eOutput0));


}

void HComputeLighting::CreateDesciptors_ssao(ID3D12Resource* pDepth, ID3D12Resource* pNormal, ID3D12Resource* pRandomVector, ID3D12Resource* pSSAO)
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;

	device->CreateShaderResourceView(
		pNormal, &srvDesc,
		m_pDesciptorHeap_SRVUAV_ssao->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_ssao::eNormal));

	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	device->CreateShaderResourceView(
		pRandomVector, &srvDesc,
		m_pDesciptorHeap_SRVUAV_ssao->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_ssao::eRandomVector));

	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	device->CreateShaderResourceView(
		pDepth, &srvDesc,
		m_pDesciptorHeap_SRVUAV_ssao->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_ssao::eDepth));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	device->CreateUnorderedAccessView(
		pSSAO, nullptr, &uavDesc,
		m_pDesciptorHeap_SRVUAV_ssao->GetCpuHandle((UINT)DescriptorHeapList_SRVUAV_ssao::eOutput));
}

void HComputeLighting::ComputeLighting()
{
	auto commandList = m_pDeviceResources->GetCommandList();

	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Light Compute");

	ID3D12DescriptorHeap* descriptorHeapArray[] = { m_pDesciptorHeap_SRVUAV_lightCalculation->Heap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeapArray), descriptorHeapArray);

	commandList->SetPipelineState(m_pComputePso_lightCalculation.Get());
	commandList->SetComputeRootSignature(m_pComputeRootSig_lightCalculation.Get());
	commandList->SetComputeRootDescriptorTable((UINT)RootSignatureList_lightCalculation::eTexuresInput,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::InputTextureStart));
	commandList->SetComputeRootDescriptorTable((UINT)RootSignatureList_lightCalculation::eTextureOutput,
		m_pDesciptorHeap_SRVUAV_lightCalculation->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_lightCalculation::eOutput0));
	commandList->SetComputeRootConstantBufferView((UINT)RootSignatureList_lightCalculation::ePassConstant,
		m_pPassConstant->GetPassConstantGpuAddress());
	commandList->SetComputeRootShaderResourceView((UINT)RootSignatureList_lightCalculation::eLight,
		m_pPassConstant->GetLightGpyAddress());


	RECT outputSize = m_pDeviceResources->GetOutputSize();

	UINT numGroupX = (UINT)ceilf((outputSize.right - outputSize.left) / 16.0f);
	UINT numGroupY = (UINT)ceilf((outputSize.bottom - outputSize.top) / 16.0f);

	commandList->Dispatch(numGroupX, numGroupY, 1);

	PIXEndEvent(commandList);
}

void HComputeLighting::DrawSSAO()
{
	auto commandList = m_pDeviceResources->GetCommandList();

	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"SSAO Compute");

	ID3D12DescriptorHeap* descriptorHeapArray[] = { m_pDesciptorHeap_SRVUAV_ssao->Heap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeapArray), descriptorHeapArray);

	commandList->SetPipelineState(m_pComputePso_ssao.Get());
	commandList->SetComputeRootSignature(m_pComputeRootSig_ssao.Get());
	commandList->SetComputeRootDescriptorTable((UINT)RootSignatureList_ssao::eTexuresInput,
		m_pDesciptorHeap_SRVUAV_ssao->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_ssao::InputTextureStart));
	commandList->SetComputeRootDescriptorTable((UINT)RootSignatureList_ssao::eTextureOutput,
		m_pDesciptorHeap_SRVUAV_ssao->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_ssao::eOutput));
	commandList->SetComputeRootConstantBufferView((UINT)RootSignatureList_ssao::ePassConstant,
		m_pPassConstant->GetPassConstantGpuAddress());

	RECT outputSize = m_pDeviceResources->GetOutputSize();

	UINT numGroupX = (UINT)ceilf((outputSize.right - outputSize.left) / 16.0f);
	UINT numGroupY = (UINT)ceilf((outputSize.bottom - outputSize.top) / 16.0f);

	commandList->Dispatch(numGroupX, numGroupY, 1);

	PIXEndEvent(commandList);

}

void HComputeLighting::ClearSSAO(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
	auto pCommandList = m_pDeviceResources->GetCommandList();

	float value[4] = { 1.0f,0.f,0.f,0.f };

	ID3D12DescriptorHeap* heaps[] = { m_pDesciptorHeap_SRVUAV_ssao->Heap() };
	pCommandList->SetDescriptorHeaps(1, heaps);


	pCommandList->ClearUnorderedAccessViewFloat(
		m_pDesciptorHeap_SRVUAV_ssao->GetGpuHandle((UINT)DescriptorHeapList_SRVUAV_ssao::eOutput),
		cpuHandle,
		pResource, value, 0, NULL);

}

void HComputeLighting::CreateDescriptorHeap()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	m_pDesciptorHeap_SRVUAV_lightCalculation
		= std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			(UINT)DescriptorHeapList_SRVUAV_lightCalculation::TotalCount);

	m_pDesciptorHeap_SRVUAV_ssao
		= std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			(UINT)DescriptorHeapList_SRVUAV_ssao::TotalCount);

}

void HComputeLighting::CreateComputePSO_lightCalculation()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {};
	computePipelineStateDesc.pRootSignature = m_pComputeRootSig_lightCalculation.Get();
	computePipelineStateDesc.CS =
	{
		g_pdeferredRender_CS,
		sizeof(g_pdeferredRender_CS)
	};
	computePipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	DX::ThrowIfFailed(device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&m_pComputePso_lightCalculation)));
}

void HComputeLighting::CreateRootSignature_lightCalculation()
{
	HRESULT hr = S_OK;

	CD3DX12_DESCRIPTOR_RANGE textureInputDescriptorTable;
	textureInputDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)DescriptorHeapList_SRVUAV_lightCalculation::InputTextureCount, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE textureOutputDescriptorTable;
	textureOutputDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (UINT)DescriptorHeapList_SRVUAV_lightCalculation::OutputTextureCount, 0, 0, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)RootSignatureList_lightCalculation::TotalCount];
	rootParameters[(UINT)RootSignatureList_lightCalculation::eTexuresInput].InitAsDescriptorTable(1, &textureInputDescriptorTable, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSignatureList_lightCalculation::eTextureOutput].InitAsDescriptorTable(1, &textureOutputDescriptorTable, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSignatureList_lightCalculation::ePassConstant].InitAsConstantBufferView(0);
	rootParameters[(UINT)RootSignatureList_lightCalculation::eLight].InitAsShaderResourceView(0,1);

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
		IID_PPV_ARGS(&m_pComputeRootSig_lightCalculation)));
}

void HComputeLighting::CreateRootSignature_ssao()
{
	HRESULT hr = S_OK;

	CD3DX12_DESCRIPTOR_RANGE textureInputDescriptorTable;
	textureInputDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)DescriptorHeapList_SRVUAV_ssao::InputTextureCount, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE textureOutputDescriptorTable;
	textureOutputDescriptorTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (UINT)DescriptorHeapList_SRVUAV_ssao::OutputTextureCount, 0, 0, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)RootSignatureList_ssao::TotalCount];
	rootParameters[(UINT)RootSignatureList_ssao::eTexuresInput].InitAsDescriptorTable(1, &textureInputDescriptorTable, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSignatureList_ssao::eTextureOutput].InitAsDescriptorTable(1, &textureOutputDescriptorTable, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSignatureList_ssao::ePassConstant].InitAsConstantBufferView(0,0, D3D12_SHADER_VISIBILITY_ALL);

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
		IID_PPV_ARGS(&m_pComputeRootSig_ssao)));
}

void HComputeLighting::CreateComputePSO_ssao()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {};
	computePipelineStateDesc.pRootSignature = m_pComputeRootSig_ssao.Get();
	computePipelineStateDesc.CS =
	{
		g_pssao_CS,
		sizeof(g_pssao_CS)
	};
	computePipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	DX::ThrowIfFailed(device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&m_pComputePso_ssao)));
}
