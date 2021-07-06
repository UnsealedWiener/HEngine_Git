#include"pch.h"
#include"HRasterize.h"
#include"d3dUtil.h"
#include"DeviceResources.h"
#include"HTextureManager.h"
#include"HPassConstant.h"
#include"HModelManager.h"
#include"HWaveEffect.h"


#include"default_VS.hlsl.h"


#include"default_DS.hlsl.h"
#include"default_HS.hlsl.h"
#include"tess_VS.hlsl.h"
#include"twoMatLerp_DS.hlsl.h"

//#include"default_PS.hlsl.h"
//#include"twoMatLerp_PS.hlsl.h"
//deferred�� ��ü
#include"default_PS_deffered.hlsl.h"
#include"colorChip_PS_deffered.hlsl.h"
#include"twoMatLerp_PS_deffered.hlsl.h"

#include"shadow_VS.hlsl.h"


using namespace HRasterizeDefines;

void HRasterize::Initialize(DeviceResources* pDeviceResources, HModelManager* pModelManager, HPassConstant* pPassConstant, HTextureManager* pTextureManager, Camera* pCamera)
{
	m_pDeviceResources = pDeviceResources;
	m_pModelManager = pModelManager;
	m_pPassConstant = pPassConstant;
	m_pTextureManager = pTextureManager;
}

void HRasterize::DrawGBuffer(bool wireframe)
{
	auto backBuffer = m_pDeviceResources->GetRenderTarget();
	auto commandList = m_pDeviceResources->GetCommandList();


	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

	{
		m_pDeviceResources->SetScreenSizeViewport();
		FLOAT zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		auto rtvCPU = m_pDescriptorHeap_RTV->GetFirstCpuHandle();
		auto dsvCPU = m_pDeviceResources->GetDepthStencilView();

		commandList->OMSetRenderTargets((UINT)DescriptorHeapList_RTV::TotalCount, &rtvCPU, TRUE, &dsvCPU);
		commandList->ClearRenderTargetView(m_pDescriptorHeap_RTV->GetCpuHandle((UINT)DescriptorHeapList_RTV::eAlbedo), zero, 0, nullptr);
		commandList->ClearRenderTargetView(m_pDescriptorHeap_RTV->GetCpuHandle((UINT)DescriptorHeapList_RTV::eMetallicRoughnessAo), zero, 0, nullptr);
		commandList->ClearRenderTargetView(m_pDescriptorHeap_RTV->GetCpuHandle((UINT)DescriptorHeapList_RTV::eNormal), zero, 0, nullptr);
		commandList->ClearRenderTargetView(m_pDescriptorHeap_RTV->GetCpuHandle((UINT)DescriptorHeapList_RTV::eEmissive), zero, 0, nullptr);


		commandList->ClearDepthStencilView(dsvCPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	PIXEndEvent(commandList);
	
	if (m_pModelManager->m_models.size() == 0|| m_pModelManager->m_visibleInstance == 0)
		return;

	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render G-Buffer");

	commandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	ID3D12DescriptorHeap* heaps[] = { m_pDescriptorHeap_SRV->Heap() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	commandList->SetGraphicsRootConstantBufferView((UINT)RootSig_Rasterize::ConstantBuffer_pass,
		m_pPassConstant->GetPassConstantGpuAddress());
	commandList->SetGraphicsRootDescriptorTable((UINT)RootSig_Rasterize::Textures,
		m_pDescriptorHeap_SRV->GetFirstGpuHandle());

	commandList->SetGraphicsRootDescriptorTable((UINT)RootSig_Rasterize::ColorChip,
		m_pDescriptorHeap_SRV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Rasterize::ColorChip));

	D3D12_VERTEX_BUFFER_VIEW vbv = m_pModelManager->GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW ibv = m_pModelManager->GetIndexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vbv);
	commandList->IASetIndexBuffer(&ibv);

	for (auto& model : m_pModelManager->m_models)
	{
		auto pModel = model.second.get();

		if (pModel->visibleInstanceCount == 0)
			continue;

		commandList->SetGraphicsRootConstantBufferView((UINT)RootSig_Rasterize::ConstantBuffer_perModel,
			pModel->perModelCB.GpuAddress());

		if (pModel->rawData.boneCount != 0)
		{
			commandList->SetGraphicsRootShaderResourceView((UINT)RootSig_Rasterize::StructuredBuffer_instancesBones,
				pModel->structuredBuffer_computeShader_boneTMs.GpuAddress());
		}

		for (const PSOTYPE& psoType : PSOTYPE())
		{
			UINT instanceCount = pModel->visibleInstances[psoType].size();

			if (instanceCount == 0)
				continue;
		
			commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			
			if(wireframe)
				commandList->SetPipelineState(m_PSO_default[PSOTYPE::WIREFRAME].Get());
			else
				commandList->SetPipelineState(m_PSO_default[psoType].Get());
			
			commandList->SetGraphicsRootShaderResourceView((UINT)RootSig_Rasterize::StructuredBuffer_perPSO,
				pModel->structuredBuffer_perPso[psoType].GpuAddress());

			commandList->SetGraphicsRootShaderResourceView((UINT)RootSig_Rasterize::StructuredBuffer_instances,
				pModel->structuredBuffer_instances[psoType].GpuAddress());

			commandList->DrawIndexedInstanced(pModel->rawData.indexData.size(), instanceCount, pModel->indexOffsetInEntireBuffer,
				pModel->vertexOffsetInEntireBuffer, 0);
		}
	}

	HWaveEffect::GetInstance()->Draw();

	//std::unordered_map<void*, std::unique_ptr<Waves>>& m_waveList
	// = HWaveEffect::GetInstance()->GetWaveList();
	//
	//for (auto& wave : m_waveList)
	//{
	//	Waves* pWave = wave.second.get();
	//	if (!pWave)
	//		continue;

	//	D3D12_VERTEX_BUFFER_VIEW vbv = pWave->GetVertexBufferView();
	//	D3D12_INDEX_BUFFER_VIEW ibv = pWave->GetIndexBufferView();

	//	commandList->IASetVertexBuffers(0, 1, &vbv);
	//	commandList->IASetIndexBuffer(&ibv);
	//}
	//commandList->mip

	PIXEndEvent(commandList);
}

void HRasterize::DrawShadow()
{
	if (m_pModelManager->m_models.size() == 0|| m_pModelManager->m_instances.size() == 0)
		return;

	auto commandList = m_pDeviceResources->GetCommandList();

	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render shadow Buffer");

	m_pDeviceResources->SetShadowMapViewport();

	auto dsv = m_pDescriptorHeap_DSV->GetCpuHandle((UINT)DescriptorHeapList_DSV::eShadow);

	commandList->OMSetRenderTargets(0, nullptr, false,
		&dsv);

	commandList->ClearDepthStencilView(dsv,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->SetPipelineState(m_shadowPSO.Get());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	D3D12_VERTEX_BUFFER_VIEW vbv = m_pModelManager->GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW ibv = m_pModelManager->GetIndexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vbv);
	commandList->IASetIndexBuffer(&ibv);

	commandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	commandList->SetGraphicsRootConstantBufferView((UINT)RootSig_Rasterize::ConstantBuffer_pass,
		m_pPassConstant->GetPassConstantGpuAddress());

	for (auto& model : m_pModelManager->m_models)
	{
		auto pModel = model.second.get();

		if (pModel->visibleInstanceCount == 0)
		continue;

		commandList->SetGraphicsRootConstantBufferView((UINT)RootSig_Rasterize::ConstantBuffer_perModel,
			pModel->perModelCB.GpuAddress());

		if (pModel->rawData.boneCount != 0)
		{
			commandList->SetGraphicsRootShaderResourceView((UINT)RootSig_Rasterize::StructuredBuffer_instancesBones,
				pModel->structuredBuffer_computeShader_boneTMs.GpuAddress());
		}

		for (const PSOTYPE& psoType : PSOTYPE())
		{
			UINT instanceCount = pModel->visibleInstances[psoType].size();

			if (instanceCount == 0)
				continue;

			commandList->SetGraphicsRootShaderResourceView((UINT)RootSig_Rasterize::StructuredBuffer_perPSO,
				pModel->structuredBuffer_perPso[psoType].GpuAddress());

			commandList->SetGraphicsRootShaderResourceView((UINT)RootSig_Rasterize::StructuredBuffer_instances,
				pModel->structuredBuffer_instances[psoType].GpuAddress());

			commandList->DrawIndexedInstanced(pModel->rawData.indexData.size(), instanceCount, pModel->indexOffsetInEntireBuffer,
				pModel->vertexOffsetInEntireBuffer, 0);
		}
	}

	m_pDeviceResources->SetScreenSizeViewport();

	PIXEndEvent(commandList);
}

void HRasterize::ClearShadowBuffer()
{
	auto commandList = m_pDeviceResources->GetCommandList();

	auto dsv = m_pDescriptorHeap_DSV->GetCpuHandle((UINT)DescriptorHeapList_DSV::eShadow);

	commandList->ClearDepthStencilView(dsv,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void HRasterize::Update()
{
	if (m_pTextureManager->GetRasterizeDirty())
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle =
			m_pTextureManager->GetMaterialDescriptorHeap()->GetFirstCpuHandle();

		m_pDescriptorHeap_SRV->WriteDescriptors(m_pDeviceResources->GetD3DDevice(),
			(UINT)SRVUAVDescriptorHeapOffset_Rasterize::Material,
			&handle,
			MAX_MATERIAL_DESCRIPTORHEAP_SIZE);

		handle = m_pTextureManager->GetSkyboxDescriptorHeap()->GetFirstCpuHandle();

		m_pDescriptorHeap_SRV->WriteDescriptors(m_pDeviceResources->GetD3DDevice(),
			(UINT)SRVUAVDescriptorHeapOffset_Rasterize::SkyBox,
			&handle,
			SKYBOXMAP_COUNT);

		handle = m_pTextureManager->GetColorChipDescriptorHeap()->GetFirstCpuHandle();

		m_pDescriptorHeap_SRV->WriteDescriptors(m_pDeviceResources->GetD3DDevice(),
			(UINT)SRVUAVDescriptorHeapOffset_Rasterize::ColorChip,
			&handle,
			COLORCHIP_TEXTURECOUNT);

		m_pTextureManager->SetRasterizeDirty(false);
	}

}

void HRasterize::CreateDeviceDependentResources()
{
	CreateDescriptorHeap();
	CreateRootSignature();
	CreatePSOs();
}

void HRasterize::CreateDescriptors (ID3D12Resource* pAlbedo, ID3D12Resource* pMetallicRoughnessAo, ID3D12Resource* pEmissive,
	ID3D12Resource* pNormal, ID3D12Resource* pShadow)
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

	device->CreateRenderTargetView(
		pAlbedo, &rtvDesc,
		m_pDescriptorHeap_RTV->GetCpuHandle((UINT)DescriptorHeapList_RTV::eAlbedo));

	device->CreateRenderTargetView(
		pMetallicRoughnessAo, &rtvDesc,
		m_pDescriptorHeap_RTV->GetCpuHandle((UINT)DescriptorHeapList_RTV::eMetallicRoughnessAo));

	device->CreateRenderTargetView(
		pEmissive, &rtvDesc,
		m_pDescriptorHeap_RTV->GetCpuHandle((UINT)DescriptorHeapList_RTV::eEmissive));

	device->CreateRenderTargetView(
		pNormal, &rtvDesc,
		m_pDescriptorHeap_RTV->GetCpuHandle((UINT)DescriptorHeapList_RTV::eNormal));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	
	device->CreateDepthStencilView(pShadow, &dsvDesc,
		m_pDescriptorHeap_DSV->GetCpuHandle((UINT)DescriptorHeapList_DSV::eShadow));
}

void HRasterize::CreateDescriptorHeap()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	m_pDescriptorHeap_RTV
		= std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			(UINT)DescriptorHeapList_RTV::TotalCount);

	m_pDescriptorHeap_SRV =
		std::make_unique<DescriptorHeap>(device, (UINT)SRVUAVDescriptorHeapOffset_Rasterize::TotalCount);

	m_pDescriptorHeap_DSV =
		std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			(UINT)DescriptorHeapList_DSV::TotalCount);
}

void HRasterize::CreateRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_MATERIAL_DESCRIPTORHEAP_SIZE, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE colorChipTable;
	colorChipTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, COLORCHIP_TEXTURECOUNT, 0, 2);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[(UINT)RootSig_Rasterize::TotalCount];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[(UINT)RootSig_Rasterize::StructuredBuffer_instances].InitAsShaderResourceView(0, 1);
	slotRootParameter[(UINT)RootSig_Rasterize::StructuredBuffer_instancesBones].InitAsShaderResourceView(1, 1);
	slotRootParameter[(UINT)RootSig_Rasterize::StructuredBuffer_perPSO].InitAsShaderResourceView(2, 1);
	slotRootParameter[(UINT)RootSig_Rasterize::ConstantBuffer_perModel].InitAsConstantBufferView(1);
	slotRootParameter[(UINT)RootSig_Rasterize::ConstantBuffer_pass].InitAsConstantBufferView(0);
	slotRootParameter[(UINT)RootSig_Rasterize::Textures].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[(UINT)RootSig_Rasterize::ColorChip].InitAsDescriptorTable(1, &colorChipTable, D3D12_SHADER_VISIBILITY_ALL);


	auto staticSamplers = d3dUtil::GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)RootSig_Rasterize::TotalCount, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
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
		IID_PPV_ARGS(m_pRootSignature.GetAddressOf())));
}

void HRasterize::CreatePSOs()
{

	//DXGI_SAMPLE_DESC msaaSampleDesc = {};
	//msaaSampleDesc.Count = m_sampleCount;
	//msaaSampleDesc.Quality = m_sampleQuality - 1;

	DXGI_SAMPLE_DESC nonMsaaSampleDesc = {};
	nonMsaaSampleDesc.Count = 1;
	nonMsaaSampleDesc.Quality = 0;

	D3D12_RASTERIZER_DESC wireframeRasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	wireframeRasterDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;

	D3D12_RASTERIZER_DESC nonWireframeRasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	nonWireframeRasterDesc.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_RASTERIZER_DESC shadowRasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	shadowRasterDesc.DepthBias = 10000;
	shadowRasterDesc.DepthBiasClamp = 0.0f;
	shadowRasterDesc.SlopeScaledDepthBias = 5.0f;

	for (const PSOTYPE& psoType : PSOTYPE())
	{
		DXGI_SAMPLE_DESC sampleDesc = {};
		D3D12_RASTERIZER_DESC rasterDesc = {};

		D3D12_SHADER_BYTECODE VS_ = {};
		D3D12_SHADER_BYTECODE PS_ = {};
		D3D12_SHADER_BYTECODE HS_ = {};
		D3D12_SHADER_BYTECODE DS_ = {};

		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType =
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		sampleDesc = nonMsaaSampleDesc;

		switch (psoType)
		{
		case PSOTYPE::DEFAULT:
			VS_.pShaderBytecode = g_pdefault_VS;
			VS_.BytecodeLength = sizeof(g_pdefault_VS);
			PS_.pShaderBytecode = g_pdefault_PS_deffered;
			PS_.BytecodeLength = sizeof(g_pdefault_PS_deffered);
			rasterDesc = nonWireframeRasterDesc;
			break;
		case PSOTYPE::COLORCHIP:
			VS_.pShaderBytecode = g_pdefault_VS;
			VS_.BytecodeLength = sizeof(g_pdefault_VS);
			PS_.pShaderBytecode = g_pcolorChip_PS_deffered;
			PS_.BytecodeLength = sizeof(g_pcolorChip_PS_deffered);
			rasterDesc = nonWireframeRasterDesc;
			break;
		case PSOTYPE::WIREFRAME:
			VS_.pShaderBytecode = g_pdefault_VS;
			VS_.BytecodeLength = sizeof(g_pdefault_VS);
			PS_.pShaderBytecode = g_pdefault_PS_deffered;
			PS_.BytecodeLength = sizeof(g_pdefault_PS_deffered);
			rasterDesc = wireframeRasterDesc;
			break;


		default:
			break;
		}

		CreatePSO(VS_, PS_, HS_, DS_, primitiveType, sampleDesc, rasterDesc, m_PSO_default[psoType].GetAddressOf());
	}

	D3D12_SHADER_BYTECODE VS_shadow;
	VS_shadow.pShaderBytecode = g_pshadow_VS;
	VS_shadow.BytecodeLength = sizeof(g_pshadow_VS);

	CreateShadowBufferPSO(VS_shadow, nonMsaaSampleDesc, shadowRasterDesc, &m_shadowPSO);
}

void HRasterize::CreateShadowBufferPSO(D3D12_SHADER_BYTECODE VS, DXGI_SAMPLE_DESC sampleDesc,
	D3D12_RASTERIZER_DESC rasterDesc, ID3D12PipelineState** outputPSO)
{
	auto device = m_pDeviceResources->GetD3DDevice();
	auto backBufferFormat = m_pDeviceResources->GetBackBufferFormat();
	auto depthBufferFormat = m_pDeviceResources->GetDepthBufferFormat();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc = {};

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENTU", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMV", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	PsoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
	PsoDesc.pRootSignature = m_pRootSignature.Get();

	PsoDesc.VS = VS;

	PsoDesc.RasterizerState = rasterDesc;
	PsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	PsoDesc.SampleMask = UINT_MAX;
	PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	PsoDesc.NumRenderTargets = 0;

	PsoDesc.SampleDesc = sampleDesc;
	PsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(outputPSO)));
}

void HRasterize::CreatePSO(D3D12_SHADER_BYTECODE VS, D3D12_SHADER_BYTECODE PS, D3D12_SHADER_BYTECODE HS, D3D12_SHADER_BYTECODE DS, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType, DXGI_SAMPLE_DESC sampleDesc, D3D12_RASTERIZER_DESC rasterDesc, ID3D12PipelineState** outputPSO)
{
	auto device = m_pDeviceResources->GetD3DDevice();
	auto backBufferFormat = m_pDeviceResources->GetBackBufferFormat();
	auto depthBufferFormat = m_pDeviceResources->GetDepthBufferFormat();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc = {};

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENTU", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMV", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	PsoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
	PsoDesc.pRootSignature = m_pRootSignature.Get();

	PsoDesc.VS = VS;

	PsoDesc.PS = PS;

	PsoDesc.HS = HS;

	PsoDesc.DS = DS;

	PsoDesc.RasterizerState = rasterDesc;
	PsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	PsoDesc.SampleMask = UINT_MAX;
	PsoDesc.PrimitiveTopologyType = primitiveType;
	PsoDesc.NumRenderTargets = (UINT)DescriptorHeapList_RTV::TotalCount;
	PsoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	PsoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	PsoDesc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	PsoDesc.RTVFormats[3] = DXGI_FORMAT_R32G32B32A32_FLOAT;

	PsoDesc.SampleDesc = sampleDesc;
	PsoDesc.DSVFormat = depthBufferFormat;

	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(outputPSO)));
}





