#include"pch.h"
#include"HGraphicPipeline.h"
#include"DeviceResources.h"
#include"HTextureManager.h"
#include"HPassConstant.h"
#include"HModelManager.h"
#include"StepTimer.h"

void HGraphicPipeline::Initialize(GraphicsMemory* pGraphicsMemory, DeviceResources* pDeviceResources, HModelManager* pModelManager,
	HPassConstant* pPassConstant, HTextureManager* pTextureManager, Camera* pCamera, StepTimer* pStepTimer)
{
	m_pDeviceResources = pDeviceResources;

	m_pModelManager = pModelManager;

	m_bRayracingSupported = pDeviceResources->IsRaytracingSupported();

	m_rasterize.Initialize(pDeviceResources, pModelManager, pPassConstant, pTextureManager, pCamera);

	m_computeLighting.Initialize(pDeviceResources, pPassConstant);

	//if (m_bRayracingSupported == true)
		m_raytracing.Initialize(pDeviceResources, pModelManager, pPassConstant, pTextureManager);

	m_pSimplePrimitiveManager = HSimplePrimitiveManager::GetInstance();

	m_pSimplePrimitiveManager->Initialize(m_pDeviceResources, pPassConstant, pCamera);

	m_pHUIManager = HUIManager::GetInstance();

	m_pHUIManager->Initialize(pGraphicsMemory, m_pDeviceResources, pTextureManager, pModelManager, pStepTimer);

}

void HGraphicPipeline::Update()
{
	m_pModelManager->UpdateAllInstancedBoneAnimResource_common();

	if (m_bRayracingSupported && m_bReflection)
	{
		m_pModelManager->UpdateResouce_raytracing();
		m_raytracing.Update();
	}

	m_pModelManager->UpdateResource_raterize();
	m_rasterize.Update();
	m_pSimplePrimitiveManager->Update();
	m_pHUIManager->Update();
}

void HGraphicPipeline::Draw()
{
	auto commandList = m_pDeviceResources->GetCommandList();
	auto backBuffer = m_pDeviceResources->GetRenderTarget();

	m_rasterize.DrawGBuffer(m_bWireFrameOn);
	if (m_bShadowOn)
	{
		m_rasterize.DrawShadow();
	}
	else
	{
		m_rasterize.ClearShadowBuffer();
	}

	if (m_bRayracingSupported && m_bReflection)
	{
		m_raytracing.Draw();
	}
	else
	{
		m_raytracing.ClearBuffer(m_reflectionBuffer.Get(),
			m_pDescriptorHeap_nonShaderVisible_forClear->
			GetCpuHandle((UINT)DescriptorList_NonShaderVisible_GraphicPipiline::eReflection));
	}


	{
		D3D12_RESOURCE_BARRIER barrier[6] = {
			CD3DX12_RESOURCE_BARRIER::Transition(m_albedoBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_metallicRoughnessAoBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_normalBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_pDeviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_shadowBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_reflectionBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),

		};
		commandList->ResourceBarrier(6, barrier);
	}

	if (m_bSSAOOn)
	{
		m_computeLighting.DrawSSAO();
	}
	else
	{
		m_computeLighting.ClearSSAO(m_ssaoBuffer.Get(),
			m_pDescriptorHeap_nonShaderVisible_forClear->
			GetCpuHandle((UINT)DescriptorList_NonShaderVisible_GraphicPipiline::eSSAO));
	}

	{
		D3D12_RESOURCE_BARRIER barrier =
			CD3DX12_RESOURCE_BARRIER::Transition(m_ssaoBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &barrier);
	}
	//compute Lighting
	m_computeLighting.ComputeLighting();

	{
		D3D12_RESOURCE_BARRIER barrier[9] = {
		CD3DX12_RESOURCE_BARRIER::Transition(m_resultBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST),
		CD3DX12_RESOURCE_BARRIER::Transition(m_albedoBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(m_metallicRoughnessAoBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(m_normalBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(m_pDeviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_shadowBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_reflectionBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(m_ssaoBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)

		};

		commandList->ResourceBarrier(9, barrier);
	}

	commandList->CopyResource(backBuffer, m_resultBuffer.Get());

	{
		D3D12_RESOURCE_BARRIER barrier[2] = {
			CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET),
			CD3DX12_RESOURCE_BARRIER::Transition(m_resultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		};

		commandList->ResourceBarrier(2, barrier);
	}

	m_pSimplePrimitiveManager->Draw(commandList);
	m_pHUIManager->Draw();
}

void HGraphicPipeline::CreateWindowSizeDependetResources()
{
	CreateGBuffers();

	CreateDesriptorsForClear();

	m_rasterize.CreateDescriptors(m_albedoBuffer.Get(), m_metallicRoughnessAoBuffer.Get(), m_normalBuffer.Get(), m_shadowBuffer.Get());
	m_computeLighting.CreateDescriptors(m_albedoBuffer.Get(), m_metallicRoughnessAoBuffer.Get(), m_normalBuffer.Get(),
		m_pDeviceResources->GetDepthStencil(), m_reflectionBuffer.Get(), m_shadowBuffer.Get(), m_randomVectorBuffer.Get(), m_ssaoBuffer.Get(), m_resultBuffer.Get());
	m_pSimplePrimitiveManager->CreateWindowSizeDependentResources();
	m_pHUIManager->CreateWindowSizeDependentResources();

	//if (m_bRayracingSupported)
		m_raytracing.CreateDescriptors(m_reflectionBuffer.Get(), m_normalBuffer.Get(), m_pDeviceResources->GetDepthStencil());
}

void HGraphicPipeline::CreateDeviceDependentResources(ResourceUploadBatch& resourceBatch)
{
	CreateDescriptorHeapForClear();

	CreateRandomVectorBuffer(resourceBatch);
	m_pSimplePrimitiveManager->CreateDeviceDependentResources();
	m_rasterize.CreateDeviceDependentResources();
	m_computeLighting.CreateDeviceDependentResources();
	m_pHUIManager->CreateDeviceDependentResource(resourceBatch);

	//if (m_bRayracingSupported)
		m_raytracing.CreateDeviceDependentResources(resourceBatch);
}



void HGraphicPipeline::CreateGBuffers()
{
	auto device = m_pDeviceResources->GetD3DDevice();
	auto size = m_pDeviceResources->GetOutputSize();
	auto backBufferFormat = m_pDeviceResources->GetBackBufferFormat();
	auto gBufferFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	auto depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	//(1)Create G-Buffer and Result Buffer
	// Determine the render target size in pixels.
	UINT backBufferWidth = std::max<UINT>(size.right - size.left, 1);
	UINT backBufferHeight = std::max<UINT>(size.bottom - size.top, 1);

	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

	// Create an Gbuffer render target.
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		gBufferFormat,
		backBufferWidth,
		backBufferHeight,
		1, // This render target view has only one texture.
		1, // Use a single mipmap level
		1
	);

	resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = gBufferFormat;
	XMVECTOR black = XMVectorZero();
	XMVECTOR white = XMVectorSet(1, 1, 1, 1);
	memcpy(optimizedClearValue.Color, &black, sizeof(float) * 4);

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&optimizedClearValue,
		IID_PPV_ARGS(m_albedoBuffer.ReleaseAndGetAddressOf())
	));

	m_albedoBuffer->SetName(L"albedoBuffer");

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&optimizedClearValue,
		IID_PPV_ARGS(m_metallicRoughnessAoBuffer.ReleaseAndGetAddressOf())
	));

	m_metallicRoughnessAoBuffer->SetName(L"metallicRoughnessAoBuffer");

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&optimizedClearValue,
		IID_PPV_ARGS(m_normalBuffer.ReleaseAndGetAddressOf())
	));

	m_normalBuffer->SetName(L"normalBuffer");

	resDesc.Format = DXGI_FORMAT_R32_FLOAT;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(m_ssaoBuffer.ReleaseAndGetAddressOf())
	));

	m_ssaoBuffer->SetName(L"ssaoBuffer");

	resDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		NULL,
		IID_PPV_ARGS(m_reflectionBuffer.ReleaseAndGetAddressOf())
	));

	m_reflectionBuffer->SetName(L"reflectionBuffer");

	resDesc.Format = NoSRGB(backBufferFormat);
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	memcpy(optimizedClearValue.Color, &white, sizeof(float) * 4);

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		NULL,
		IID_PPV_ARGS(m_resultBuffer.ReleaseAndGetAddressOf())
	));

	m_resultBuffer->SetName(L"resultBuffer");

	//create ShadowBuffer
	D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		2048,
		2048,
		1, // This depth stencil view has only one texture.
		0, // Use a single mipmap level.
		1
	);

	depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	resDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(m_shadowBuffer.ReleaseAndGetAddressOf())
	));

	m_shadowBuffer->SetName(L"shadowBuffer");

}



void HGraphicPipeline::CreateDescriptorHeapForClear()
{
	m_pDescriptorHeap_nonShaderVisible_forClear =
		std::make_unique<DescriptorHeap>(m_pDeviceResources->GetD3DDevice(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			(UINT)DescriptorList_NonShaderVisible_GraphicPipiline::TotalCount);
}

void HGraphicPipeline::CreateDesriptorsForClear()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	device->CreateUnorderedAccessView(m_reflectionBuffer.Get(), nullptr, &uavDesc,
		m_pDescriptorHeap_nonShaderVisible_forClear->
		GetCpuHandle((UINT)DescriptorList_NonShaderVisible_GraphicPipiline::eReflection));
	device->CreateUnorderedAccessView(m_ssaoBuffer.Get(), nullptr, &uavDesc,
		m_pDescriptorHeap_nonShaderVisible_forClear->
		GetCpuHandle((UINT)DescriptorList_NonShaderVisible_GraphicPipiline::eSSAO));
}

void HGraphicPipeline::CreateRandomVectorBuffer(ResourceUploadBatch& resourceBatch)
{
	auto pDevice = m_pDeviceResources->GetD3DDevice();

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = 512;
	texDesc.Height = 512;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CD3DX12_HEAP_PROPERTIES heapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&heapDesc,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_randomVectorBuffer)));

	Color* initData = new Color[512 * 512];
	for (int i = 0; i < 512; i++)
	{
		for (int j = 0; j < 512; j++)
		{
			initData[512 * i + j] = Color((float)(rand() / (float)RAND_MAX),
				(float)(rand() / (float)RAND_MAX), (float)(rand() / (float)RAND_MAX), 0.f);
		}


	}

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = 512 * sizeof(Color);
	subResourceData.SlicePitch = subResourceData.RowPitch * 512;

	resourceBatch.Upload(m_randomVectorBuffer.Get(), 0, &subResourceData, 1);

	resourceBatch.Transition(m_randomVectorBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	delete[]initData;
}
