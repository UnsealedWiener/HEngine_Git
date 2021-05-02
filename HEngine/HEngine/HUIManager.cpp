#include"pch.h"
#include"HUIManager.h"
#include"DeviceResources.h"
#include"HTextureManager.h"
#include"d3dUtil.h"

#include"ui_PS.hlsl.h"
#include"ui_VS.hlsl.h"


HUIManager* HUIManager::GetInstance()
{
	static HUIManager uiManager;

	return &uiManager;
}

void HUIManager::Initialize(GraphicsMemory* pGraphicMemory, DeviceResources* pDeviceResources, HTextureManager* pTextureManager)
{
	m_pDeviceResources = pDeviceResources;
	m_pTextureManager = pTextureManager;
	m_pGraphicMemory = pGraphicMemory;
}

void HUIManager::Draw()
{
	if (m_uiList.size() == 0)
		return;

	m_pDeviceResources->SetUiViewPort();

	auto commandList = m_pDeviceResources->GetCommandList();
	auto rtv = m_pDeviceResources->GetRenderTargetView();
	auto dsv = m_pDeviceResources->GetDepthStencilView();
	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	commandList->SetPipelineState(m_PSO.Get());
	commandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_VERTEX_BUFFER_VIEW vbv = GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW ibv = GetIndexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vbv);
	commandList->IASetIndexBuffer(&ibv);

	ID3D12DescriptorHeap* heaps[] = { m_pTextureManager->GetSpriteDescriptorHeap()->Heap() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	commandList->SetGraphicsRootShaderResourceView((UINT)RootSig_UI::eStructuredBufferPerInstance,
		m_structuredBuffer_perInstance.GpuAddress());

	commandList->SetGraphicsRootDescriptorTable((UINT)RootSig_UI::eTextures,
		m_pTextureManager->GetSpriteDescriptorHeap()->GetFirstGpuHandle());

	commandList->DrawIndexedInstanced(6, m_uiList.size(), 0,
		0, 0);
}

void HUIManager::Update()
{
	RECT output = m_pDeviceResources->GetOutputSize();
	float width = output.right - output.left;
	float height = output.bottom - output.top;

	std::vector<HStructuredBuffer_PerInstance_UI> perInstanceDatas;

	for (std::pair<void *const, std::unique_ptr<HUI>>& e : m_uiList)
	{
		HStructuredBuffer_PerInstance_UI perInstanceData;

		HUI* pHUI = e.second.get();

		Matrix world;
		if (pHUI->bIsAbsolutePosition)
		{
			Matrix scale = Matrix::CreateScale(pHUI->width, pHUI->height, 1.f);
			Matrix translation = Matrix::CreateTranslation(pHUI->leftTopPosition);
			world *= scale;
			world *= translation;
		}
		else
		{
			Matrix scale = Matrix::CreateScale(pHUI->width * width, pHUI->height * height, 1.f);

			Vector3 Translation = pHUI->leftTopPosition;
			Translation.x = Translation.x * width;
			Translation.y = Translation.y * height;

			Matrix translation = Matrix::CreateTranslation(Translation);
			world *= scale;
			world *= translation;
		}

		XMMATRIX matProj = XMMatrixOrthographicOffCenterLH(0.f, width, height, 0.f, 0.f, 1000.f);
		Matrix proj = Matrix(matProj);

		Matrix worldProj;
		worldProj *= world;
		worldProj *= proj;
		worldProj = worldProj.Transpose();

		perInstanceData.worldProjTM = worldProj;

		if (pHUI->pTextureIndex.lock().get() != nullptr)
			perInstanceData.textureIndex = *pHUI->pTextureIndex.lock().get();
		else
			perInstanceData.textureIndex = 0;

		perInstanceData.opacity = pHUI->opacity;

		perInstanceDatas.push_back(perInstanceData);

	}

	if (perInstanceDatas.size() != 0)
	{
		UINT bufferSize = sizeof(HStructuredBuffer_PerInstance_UI) * perInstanceDatas.size();

		SharedGraphicsResource graphicResource =
			m_pGraphicMemory->Allocate(bufferSize);

		memcpy(graphicResource.Memory(),
			perInstanceDatas.data(),
			bufferSize);

		m_structuredBuffer_perInstance.Reset(graphicResource);
	}
}

void HUIManager::CreateDeviceDependentResource()
{
	CreateRootSignature_lightCalculation();
	CreatePSO();
	CreateVertexIndexBuffer();
}

HUIData* HUIManager::CreateUI()
{
	std::unique_ptr<HUI> pUi = std::make_unique<HUI>();
	pUi->managerController = HManagerController(this, &m_uiList, pUi.get());

	HUIData* returnPtr = pUi.get();

	m_uiList[returnPtr] = std::move(pUi);

	return returnPtr;
}

void HUIManager::CreateRootSignature_lightCalculation()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SPRITE_DESCRIPTORHEAP_SIZE, 0, 1);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[(UINT)RootSig_UI::TotalCount];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[(UINT)RootSig_UI::eStructuredBufferPerInstance].InitAsShaderResourceView(0, 0);
	slotRootParameter[(UINT)RootSig_UI::eTextures].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);


	auto staticSamplers = d3dUtil::GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)RootSig_UI::TotalCount, slotRootParameter,
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

void HUIManager::CreatePSO()
{
	auto device = m_pDeviceResources->GetD3DDevice();
	auto backBufferFormat = m_pDeviceResources->GetBackBufferFormat();
	auto depthBufferFormat = m_pDeviceResources->GetDepthBufferFormat();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc = {};

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	PsoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
	PsoDesc.pRootSignature = m_pRootSignature.Get();

	PsoDesc.VS.pShaderBytecode = g_pui_VS;
	PsoDesc.VS.BytecodeLength = sizeof(g_pui_VS);
	PsoDesc.PS.pShaderBytecode = g_pui_PS;
	PsoDesc.PS.BytecodeLength = sizeof(g_pui_PS);
	PsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PsoDesc.BlendState.RenderTarget[0].BlendEnable = true;
	PsoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	PsoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	PsoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	PsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	PsoDesc.SampleMask = UINT_MAX;
	PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PsoDesc.NumRenderTargets = 1;
	PsoDesc.RTVFormats[0] = backBufferFormat;
	PsoDesc.SampleDesc.Count = 1;
	PsoDesc.SampleDesc.Quality = 0;
	PsoDesc.DSVFormat = depthBufferFormat;

	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&m_PSO)));

}

void HUIManager::CreateVertexIndexBuffer()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	std::vector<HVertex_UI> vertexList;

	HVertex_UI v0 = { Vector3(0,0,0), Vector2(0,0) };
	HVertex_UI v1 = { Vector3(1,0,0), Vector2(1,0) };
	HVertex_UI v2 = { Vector3(0, 1,0), Vector2(0,1) };
	HVertex_UI v3 = { Vector3(1, 1,0), Vector2(1,1) };

	vertexList.push_back(v0);
	vertexList.push_back(v1);
	vertexList.push_back(v2);
	vertexList.push_back(v3);

	std::vector<std::uint32_t> indexList;
	indexList.push_back(0);
	indexList.push_back(1);
	indexList.push_back(2);
	indexList.push_back(2);
	indexList.push_back(1);
	indexList.push_back(3);


	ResourceUploadBatch batch(device);
	batch.Begin();

	CreateStaticBuffer(device, batch, vertexList,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		| D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		m_verticesResource.ReleaseAndGetAddressOf());

	CreateStaticBuffer(device, batch, indexList,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		| D3D12_RESOURCE_STATE_INDEX_BUFFER,
		m_indicesResource.ReleaseAndGetAddressOf());

	auto uploadResourcesFinished = batch.End(m_pDeviceResources->GetCommandQueue());
	uploadResourcesFinished.wait();
}

D3D12_VERTEX_BUFFER_VIEW HUIManager::GetVertexBufferView()
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = m_verticesResource->GetGPUVirtualAddress();
	vbv.StrideInBytes = sizeof(HVertex_UI);
	vbv.SizeInBytes = sizeof(HVertex_UI) * 4;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW HUIManager::GetIndexBufferView()
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = m_indicesResource->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R32_UINT;
	ibv.SizeInBytes = sizeof(std::uint32_t) * 6;
	return ibv;
}
