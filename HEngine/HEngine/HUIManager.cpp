#include"pch.h"
#include"HUIManager.h"
#include"DeviceResources.h"
#include"HTextureManager.h"
#include"HModelManager.h"
#include"StepTimer.h"
#include"d3dUtil.h"

#include"ui_PS.hlsl.h"
#include"ui_VS.hlsl.h"


HUIManager* HUIManager::GetInstance()
{
	static HUIManager uiManager;

	return &uiManager;
}

void HUIManager::Initialize(GraphicsMemory* pGraphicMemory, DeviceResources* pDeviceResources, HTextureManager* pTextureManager,
	HModelManager* pModelManager, StepTimer* pStepTimer)
{
	m_pDeviceResources = pDeviceResources;
	m_pTextureManager = pTextureManager;
	m_pGraphicMemory = pGraphicMemory;
	m_pModelManager = pModelManager;
	m_pStepTimer = pStepTimer;
}

void HUIManager::Draw()
{
	Draw2DSprite();
	DrawFont();
}

void HUIManager::Update()
{
	RECT output = m_pDeviceResources->GetOutputSize();
	float width = output.right - output.left;
	float height = output.bottom - output.top;

	std::vector<HStructuredBuffer_PerInstance_UI> perInstanceDatas;

	for (std::pair<void *, std::unique_ptr<HUI>>& e : m_uiList)
	//for (std::unique_ptr<HUI>& e : m_uiList)
	{
		HUI* pHUI = e.second.get();
		//HUI* pHUI = e.get();


		if (!pHUI->bDrawOnOff)
			continue;

		HStructuredBuffer_PerInstance_UI perInstanceData;

		Matrix world;
		if (pHUI->bIsAbsolutePosition)
		{
			Matrix scale = Matrix::CreateScale(pHUI->size.x, pHUI->size.y, 1.f);
			Matrix translation = Matrix::CreateTranslation(pHUI->leftTopPosition);
			world *= scale;
			world *= translation;
		}
		else
		{
			Matrix scale = Matrix::CreateScale(pHUI->size.x * width, pHUI->size.y * height, 1.f);

			Vector3 Translation = pHUI->leftTopPosition;
			Translation.x = Translation.x * width;
			Translation.y = Translation.y * height;

			Matrix translation = Matrix::CreateTranslation(Translation);
			world *= scale;
			world *= translation;
		}

		XMMATRIX matProj = XMMatrixOrthographicOffCenterLH(0.f, width, height, 0.f, 0.f, 1.f);
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

	m_spriteInstancesCnt = perInstanceDatas.size();

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

void HUIManager::CreateDeviceDependentResource(ResourceUploadBatch& resourceBatch)
{
	CreateRootSignature_UI();
	CreatePSO();
	CreateVertexIndexBuffer();

	CreateSpriteBatchForFont(resourceBatch);
	auto device = m_pDeviceResources->GetD3DDevice();
	m_FontDescriptorHeap = std::make_unique<DescriptorHeap>(device, 1);
}

void HUIManager::CreateWindowSizeDependentResources()
{
	auto viewport = m_pDeviceResources->GetScreenViewport();
	m_sprites->SetViewport(viewport);
}

HUIData* HUIManager::CreateUI()
{
	std::unique_ptr<HUI> pUi = std::make_unique<HUI>();
	pUi->managerController = HManagerController_vector(this, &m_uiList, pUi.get());

	HUIData* returnPtr = pUi.get();
	//auto tmep = m_uiList.end();
	//m_uiList.push_back(std::move(pUi));

	 

	//m_uiList[returnPtr] = std::move(pUi);
	m_uiList.push_back(std::make_pair(returnPtr, std::move(pUi)));

	

	return returnPtr;
}

void HUIManager::AddDebugString(DebugString debugString)
{
	m_debugStrings.push_back(debugString);
}

void HUIManager::CreateRootSignature_UI()
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
	PsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
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
	HVertex_UI v1 = { Vector3(1,0,0), Vector2(0.99f,0) };
	HVertex_UI v2 = { Vector3(0, 1,0), Vector2(0,0.99f) };
	HVertex_UI v3 = { Vector3(1, 1,0), Vector2(0.99,0.99f) };

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

void HUIManager::CreateSpriteBatchForFont(ResourceUploadBatch& resourceBatch)
{
	auto device = m_pDeviceResources->GetD3DDevice();

	RenderTargetState rtState(m_pDeviceResources->GetBackBufferFormat(), m_pDeviceResources->GetDepthBufferFormat());

	CD3DX12_DEPTH_STENCIL_DESC  defaultDepth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	defaultDepth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	SpriteBatchPipelineStateDescription pd(rtState,nullptr, &defaultDepth);

	m_sprites = std::make_unique<SpriteBatch>(device, resourceBatch, pd);


}

void HUIManager::LoadFont(const WCHAR* spriteFontFile, ResourceUploadBatch& resourceBatch)
{
	m_font = std::make_unique<SpriteFont>(m_pDeviceResources->GetD3DDevice(),
		resourceBatch,
		spriteFontFile,
		m_FontDescriptorHeap->GetFirstCpuHandle(),
		m_FontDescriptorHeap->GetFirstGpuHandle());
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

void HUIManager::Draw2DSprite()
{
	if (m_spriteInstancesCnt == 0)
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

	commandList->DrawIndexedInstanced(6, m_spriteInstancesCnt, 0,
		0, 0);
}

void HUIManager::DrawFont()
{
	RECT output = m_pDeviceResources->GetOutputSize();
	float width = output.right - output.left;
	float height = output.bottom - output.top;


	auto commandList = m_pDeviceResources->GetCommandList();
	auto rtvDescriptor = m_pDeviceResources->GetRenderTargetView();
	auto dsv = m_pDeviceResources->GetDepthStencilView();

	commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsv);

	ID3D12DescriptorHeap* heaps1[] = { m_FontDescriptorHeap->Heap() };
	commandList->SetDescriptorHeaps(_countof(heaps1), heaps1);

	m_sprites->Begin(commandList);

	m_pDeviceResources->SetUiViewPort();

	DXGI_QUERY_VIDEO_MEMORY_INFO result;
	m_pDeviceResources->GerAdapter3()->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &result);
	std::string gpuMemory = "GraphicMemory\nbudget: " + std::to_string(result.Budget) +
		"\ncurrentUsage: " + std::to_string(result.CurrentUsage) +
		"\navailableReservation: " + std::to_string(result.AvailableForReservation) +
		"\ncurrentReservation: " + std::to_string(result.CurrentReservation);

	UINT fps = m_pStepTimer->GetFramesPerSecond();

	std::string strFPS = "FPS : " + std::to_string(fps);

	int startPosY = 500;
	std::string totalObject = "Total Object : " + std::to_string(m_pModelManager->GetTotalInstanceCnt());
	std::string visibleObject = "Visible Object : " + std::to_string(m_pModelManager->GetVisibleInstanceCnt());

	if (m_font)
	{

#ifdef _DEBUG
		m_font->DrawString(m_sprites.get(), strFPS.c_str(), XMFLOAT2(100, startPosY), Colors::Black, 0, XMFLOAT2(0, 0), XMFLOAT2(1, 1), DirectX::SpriteEffects::SpriteEffects_None, 0.0);
		m_font->DrawString(m_sprites.get(), totalObject.c_str(), XMFLOAT2(100, startPosY += 30), Colors::Black, 0, XMFLOAT2(0, 0), XMFLOAT2(1, 1), DirectX::SpriteEffects::SpriteEffects_None, 0.0);
		m_font->DrawString(m_sprites.get(), visibleObject.c_str(), XMFLOAT2(100, startPosY += 30), Colors::Black, 0, XMFLOAT2(0, 0), XMFLOAT2(1, 1), DirectX::SpriteEffects::SpriteEffects_None, 0.0);
		m_font->DrawString(m_sprites.get(), gpuMemory.c_str(), XMFLOAT2(100, startPosY += 30), Colors::Black, 0, XMFLOAT2(0, 0), XMFLOAT2(1, 1), DirectX::SpriteEffects::SpriteEffects_None, 0.0);
#endif // DEBUG

		while (!m_debugStrings.empty())
		{
			DebugString& debugString = m_debugStrings.front();

			if (debugString.bIsAbsolutePosition)
			{
				m_font->DrawString(m_sprites.get(), debugString.message.c_str(), XMFLOAT2(debugString.pos.x, debugString.pos.y),
					debugString.color, 0, XMFLOAT2(0, 0), XMFLOAT2(debugString.size.x, debugString.size.y), DirectX::SpriteEffects::SpriteEffects_None, debugString.pos.z);
				
			}
			else
			{
				m_font->DrawString(m_sprites.get(), debugString.message.c_str(), XMFLOAT2(debugString.pos.x * width, debugString.pos.y* height),
					debugString.color, 0, XMFLOAT2(0, 0), XMFLOAT2(debugString.size.x, debugString.size.y), DirectX::SpriteEffects::SpriteEffects_None, debugString.pos.z);
			}
			m_debugStrings.pop_front();
		}
	}

	m_sprites->End();
}
