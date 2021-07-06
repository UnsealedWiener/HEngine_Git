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

	m_postProcess = HPostProcess::GetInstance();
	m_postProcess->Initialize(pDeviceResources, pPassConstant);

	//if (m_bRayracingSupported == true)
	m_raytracing.Initialize(pDeviceResources, pGraphicsMemory, pModelManager, pPassConstant, pTextureManager);

	m_pSimplePrimitiveManager = HSimplePrimitiveManager::GetInstance();

	m_pSimplePrimitiveManager->Initialize(m_pDeviceResources, pPassConstant, pCamera);

	m_pHUIManager = HUIManager::GetInstance();

	m_pHUIManager->Initialize(pGraphicsMemory, m_pDeviceResources, pTextureManager, pModelManager, pStepTimer);

	m_pWaveEffect = HWaveEffect::GetInstance();
	m_pWaveEffect->Initialize(pPassConstant);
	//HWaveData* pTemp =
	//	m_pWaveEffect->CreateWave(200, 200, 1.f, 0.03f, 4.0f, 0.1f);
	//pTemp->Delete();
}

void HGraphicPipeline::Update(float dTime)
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
	m_pWaveEffect->Update(dTime);
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
		D3D12_RESOURCE_BARRIER barrier[7] = {
			CD3DX12_RESOURCE_BARRIER::Transition(m_albedoBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_metallicRoughnessAoEmissiveBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_normalBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_pDeviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_shadowBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_reflectionBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_emissiveBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),

		};
		commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
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


	if (m_bDOFOn)
	{
		{
			D3D12_RESOURCE_BARRIER barrier[8] = {
				//CD3DX12_RESOURCE_BARRIER::Transition(m_fianlResultBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(m_sceneBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(m_albedoBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				CD3DX12_RESOURCE_BARRIER::Transition(m_metallicRoughnessAoEmissiveBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				CD3DX12_RESOURCE_BARRIER::Transition(m_emissiveBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				CD3DX12_RESOURCE_BARRIER::Transition(m_normalBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				CD3DX12_RESOURCE_BARRIER::Transition(m_shadowBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
				CD3DX12_RESOURCE_BARRIER::Transition(m_reflectionBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
				CD3DX12_RESOURCE_BARRIER::Transition(m_ssaoBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)

			};

			commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
		}



		m_postProcess->DrawVerticalBlur();

		{
			D3D12_RESOURCE_BARRIER barrier[1] = {
				CD3DX12_RESOURCE_BARRIER::Transition(m_verticalBlurBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			};
			commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
		}

		m_postProcess->DrawHorizonBlur();

		{
			D3D12_RESOURCE_BARRIER barrier[2] = {
				CD3DX12_RESOURCE_BARRIER::Transition(m_verticalBlurBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
				CD3DX12_RESOURCE_BARRIER::Transition(m_horizonBlurBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			};
			commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
		}

		m_postProcess->DrawDOF();

		{
			D3D12_RESOURCE_BARRIER barrier[5] = {
				//CD3DX12_RESOURCE_BARRIER::Transition(m_fianlResultBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST),
				CD3DX12_RESOURCE_BARRIER::Transition(m_fianlResultBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(m_sceneBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
				CD3DX12_RESOURCE_BARRIER::Transition(m_pDeviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
				CD3DX12_RESOURCE_BARRIER::Transition(m_horizonBlurBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

			};

			commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
		}

		commandList->CopyResource(backBuffer, m_fianlResultBuffer.Get());

		{
			D3D12_RESOURCE_BARRIER barrier[2] = {
				CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET),
				CD3DX12_RESOURCE_BARRIER::Transition(m_fianlResultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
			};

			commandList->ResourceBarrier(2, barrier);
		}
	}
	else
	{
		{
			D3D12_RESOURCE_BARRIER barrier[10] = {
				//CD3DX12_RESOURCE_BARRIER::Transition(m_fianlResultBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST),
				CD3DX12_RESOURCE_BARRIER::Transition(m_sceneBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(m_albedoBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				CD3DX12_RESOURCE_BARRIER::Transition(m_metallicRoughnessAoEmissiveBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				CD3DX12_RESOURCE_BARRIER::Transition(m_emissiveBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				CD3DX12_RESOURCE_BARRIER::Transition(m_normalBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
				CD3DX12_RESOURCE_BARRIER::Transition(m_shadowBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
				CD3DX12_RESOURCE_BARRIER::Transition(m_reflectionBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
				CD3DX12_RESOURCE_BARRIER::Transition(m_ssaoBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
				CD3DX12_RESOURCE_BARRIER::Transition(m_pDeviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE)

			};

			commandList->ResourceBarrier(ARRAYSIZE(barrier), barrier);
		}

		commandList->CopyResource(backBuffer, m_sceneBuffer.Get());

		{
			D3D12_RESOURCE_BARRIER barrier[2] = {
				CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET),
				CD3DX12_RESOURCE_BARRIER::Transition(m_sceneBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
			};

			commandList->ResourceBarrier(2, barrier);
		}
	}




	m_pSimplePrimitiveManager->Draw(commandList);
	m_pHUIManager->Draw();
}

void HGraphicPipeline::CreateWindowSizeDependetResources()
{
	CreateGBuffers();

	CreateDesriptorsForClear();

	m_rasterize.CreateDescriptors(m_albedoBuffer.Get(), m_metallicRoughnessAoEmissiveBuffer.Get(), m_emissiveBuffer.Get(), m_normalBuffer.Get(), m_shadowBuffer.Get());
	m_computeLighting.CreateDescriptors(m_albedoBuffer.Get(), m_metallicRoughnessAoEmissiveBuffer.Get(), m_normalBuffer.Get(), m_emissiveBuffer.Get(),
		m_pDeviceResources->GetDepthStencil(), m_reflectionBuffer.Get(), m_shadowBuffer.Get(), m_randomVectorBuffer.Get(), m_ssaoBuffer.Get(), m_sceneBuffer.Get());
	m_pSimplePrimitiveManager->CreateWindowSizeDependentResources();
	m_pHUIManager->CreateWindowSizeDependentResources();

	m_postProcess->CreateDescriptors(m_pDeviceResources->GetDepthStencil(), m_sceneBuffer.Get(), m_fianlResultBuffer.Get(), m_verticalBlurBuffer.Get(),
		m_horizonBlurBuffer.Get());

	//if (m_bRayracingSupported)
	m_raytracing.CreateDescriptors(m_reflectionBuffer.Get(), m_normalBuffer.Get(), m_pDeviceResources->GetDepthStencil(),
		m_metallicRoughnessAoEmissiveBuffer.Get(), m_randomVectorBuffer.Get());
}

void HGraphicPipeline::CreateDeviceDependentResources(ResourceUploadBatch& resourceBatch)
{
	CreateDescriptorHeapForClear();

	CreateRandomVectorBuffer(resourceBatch);
	m_pSimplePrimitiveManager->CreateDeviceDependentResources();
	m_rasterize.CreateDeviceDependentResources();
	m_computeLighting.CreateDeviceDependentResources();
	m_postProcess->CreateDeviceDependentResources();
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
		IID_PPV_ARGS(m_metallicRoughnessAoEmissiveBuffer.ReleaseAndGetAddressOf())
	));

	m_metallicRoughnessAoEmissiveBuffer->SetName(L"metallicRoughnessAoBuffer");

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&optimizedClearValue,
		IID_PPV_ARGS(m_emissiveBuffer.ReleaseAndGetAddressOf())
	));

	m_emissiveBuffer->SetName(L"emissiveBuffer");

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

	//memcpy(optimizedClearValue.Color, &white, sizeof(float) * 4);

	resDesc.Format = NoSRGB(backBufferFormat);
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		NULL,
		IID_PPV_ARGS(m_fianlResultBuffer.ReleaseAndGetAddressOf())
	));

	m_fianlResultBuffer->SetName(L"finalResultBuffer");

	D3D12_RESOURCE_DESC resDesc_resultBuffer = CD3DX12_RESOURCE_DESC::Tex2D(
		gBufferFormat,
		backBufferWidth,
		backBufferHeight,
		1, // This render target view has only one texture.
		1, // Use a single mipmap level
		1
	);

	resDesc_resultBuffer.Format = NoSRGB(backBufferFormat);
	resDesc_resultBuffer.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc_resultBuffer,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		NULL,
		IID_PPV_ARGS(m_sceneBuffer.ReleaseAndGetAddressOf())
	));

	m_sceneBuffer->SetName(L"sceneBuffer");

	D3D12_RESOURCE_DESC resDesc_blur = CD3DX12_RESOURCE_DESC::Tex2D(
		gBufferFormat,
		backBufferWidth / 2,
		backBufferHeight / 2,
		1, // This render target view has only one texture.
		1, // Use a single mipmap level
		1
	);

	resDesc_blur.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc_blur,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		NULL,
		IID_PPV_ARGS(m_verticalBlurBuffer.ReleaseAndGetAddressOf())
	));

	m_verticalBlurBuffer->SetName(L"verticalBlurBuffer");

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc_blur,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		NULL,
		IID_PPV_ARGS(m_horizonBlurBuffer.ReleaseAndGetAddressOf())
	));

	m_horizonBlurBuffer->SetName(L"horizonBlurBuffer");



	/*resDesc.Width = resDesc.Width / 2;
	resDesc.Height = resDesc.Height / 2;


	DX::ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		NULL,
		IID_PPV_ARGS(m_resultBuffer_quaterSize.ReleaseAndGetAddressOf())
	));

	m_resultBuffer->SetName(L"resultBuffer_quaterSize");*/


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

	//resDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//optimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

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
