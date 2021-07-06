#include"pch.h"
#include"HRaytracing.h"
#include"DeviceResources.h"
#include"HTextureManager.h"
#include"HModelManager.h"
#include"HPassConstant.h"
#include"Raytracing.hlsl.h"
#include"RayGeneration.hlsl.h"
#include"Miss.hlsl.h"
#include"HitGroup0.hlsl.h"
#include"HitGroup1.hlsl.h"
#include"d3dUtil.h"



//#include"SkinAnim_Raytracing.hlsl.h"

void HRaytracing::Initialize(DeviceResources* pDeviceResources, GraphicsMemory* pGraphicsMemory, HModelManager* pModelManager, HPassConstant* pPassConstant, HTextureManager* pTextureManager)
{
	m_pDeviceResources = pDeviceResources;
	m_pModelManager = pModelManager;
	m_pPassConstant = pPassConstant;
	m_pTextureManager = pTextureManager;
	m_pGraphicsMemory = pGraphicsMemory;
}

void HRaytracing::CreateDeviceDependentResources(ResourceUploadBatch& resourceBatch)
{
	if (m_pDeviceResources->IsRaytracingSupported())
	{
		CreateGlobalRootSignature();
		CreateLocalRootSignature_Raygen();
		CreateLocalRootSignature_Miss();
		CreateLocalRootSignature_HitGroup0();
		CreateLocalRootSignature_HitGroup1();

		CreateRaytracingStateObject();
		
	}

	m_pDescriptorHeap_SRVUAV =
		std::make_unique<DescriptorHeap>(m_pDeviceResources->GetD3DDevice(), (UINT)SRVUAVDescriptorHeapOffset_Raytracing::TotalCount);
}

//void HRaytracing::CreateWindowSizeDepedentResources()
//{
//	auto size = m_pDeviceResources->GetOutputSize();
//	auto device = m_pDeviceResources->GetD3DDevice();
//	auto backBufferFormat = m_pDeviceResources->GetBackBufferFormat();
//
//	// Determine the render target size in pixels.
//	UINT backBufferWidth = std::max<UINT>(size.right - size.left, 1);
//	UINT backBufferHeight = std::max<UINT>(size.bottom - size.top, 1);
//
//	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
//
//	// Create an MSAA render target.
//	D3D12_RESOURCE_DESC DXRDesc = CD3DX12_RESOURCE_DESC::Tex2D(
//		NoSRGB(backBufferFormat),//UAV는 sRGB포멧으로 만들 수 없다. 쉐이더 코드에서 감마콜렉션을 해줘야한다.
//		backBufferWidth,
//		backBufferHeight,
//		1, // This render target view has only one texture.
//		1, // Use a single mipmap level
//		1
//	);
//
//	DXRDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
//
//	DX::ThrowIfFailed(device->CreateCommittedResource(
//		&heapProperties,
//		D3D12_HEAP_FLAG_NONE,
//		&DXRDesc,
//		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
//		nullptr,
//		IID_PPV_ARGS(m_raytracingOutput.ReleaseAndGetAddressOf())
//	));
//
//	m_raytracingOutput->SetName(L"DXR Output");
//
//	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
//	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
//
//	device->CreateUnorderedAccessView(m_raytracingOutput.Get(), nullptr, &uavDesc,
//		m_pDescriptorHeap_SRVUAV->GetCpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::OutputBuffer));
//
//}

void HRaytracing::CreateDescriptors(ID3D12Resource* pReflection, ID3D12Resource* pNormal, ID3D12Resource* pDepth, ID3D12Resource * pMetallicRoughnessAoEmissive,
	ID3D12Resource* pRandomTexture)
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	device->CreateUnorderedAccessView(pReflection, nullptr, &uavDesc,
		m_pDescriptorHeap_SRVUAV->GetCpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::OutputBuffer));

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
		m_pDescriptorHeap_SRVUAV->GetCpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::Normal));

	device->CreateShaderResourceView(
		pMetallicRoughnessAoEmissive, &srvDesc,
		m_pDescriptorHeap_SRVUAV->GetCpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::MetallicRoughnessAoEmissive));

	device->CreateShaderResourceView(
		pRandomTexture, &srvDesc,
		m_pDescriptorHeap_SRVUAV->GetCpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::RandomTexture));

	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	device->CreateShaderResourceView(
		pDepth, &srvDesc,
		m_pDescriptorHeap_SRVUAV->GetCpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::DepthMap));

	if (m_pDeviceResources->IsRaytracingSupported())
	InitializeShaderBindingTable();
}

void HRaytracing::Update()
{
	if (m_pTextureManager->GetRaytracingDirty())
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle =
			m_pTextureManager->GetMaterialDescriptorHeap()->GetFirstCpuHandle();

		m_pDescriptorHeap_SRVUAV->WriteDescriptors(m_pDeviceResources->GetD3DDevice(),
			(UINT)SRVUAVDescriptorHeapOffset_Raytracing::Material,
			&handle,
			MAX_MATERIAL_DESCRIPTORHEAP_SIZE);

		handle = m_pTextureManager->GetSkyboxDescriptorHeap()->GetFirstCpuHandle();

		m_pDescriptorHeap_SRVUAV->WriteDescriptors(m_pDeviceResources->GetD3DDevice(),
			(UINT)SRVUAVDescriptorHeapOffset_Raytracing::Skybox,
			&handle,
			SKYBOXMAP_COUNT);

		handle = m_pTextureManager->GetColorChipDescriptorHeap()->GetFirstCpuHandle();
		
		m_pDescriptorHeap_SRVUAV->WriteDescriptors(m_pDeviceResources->GetD3DDevice(),
			(UINT)SRVUAVDescriptorHeapOffset_Raytracing::ColorChip,
			&handle,
			COLORCHIP_TEXTURECOUNT);


		m_pTextureManager->SetRaytracingDirty(false);

	}

	if (m_pModelManager->m_bRaytracingDirty|| m_pTextureManager->GetRaytracingDirty())
	{
		//UpdateShaderBindingTable();
		
		m_pModelManager->m_bRaytracingDirty = false;
	}
	
}

void HRaytracing::Draw()
{
	if (m_pModelManager->m_visibleInstance == 0)
		return;


	auto pCommandList = m_pDeviceResources->GetRaytracingCommandList();
	UINT currentFrame = m_pDeviceResources->GetCurrentFrameIndex();
	auto size = m_pDeviceResources->GetOutputSize();
	auto backBuffer = m_pDeviceResources->GetRenderTarget();

	m_pModelManager->UpdateVertexBuffer_raytracing(pCommandList);
	m_pModelManager->UpdateRaytracingBottomLevelAccelerationStructure_raytracing(pCommandList);
	m_pModelManager->UpdateRaytracingTopLevelAccelerationStructure_raytracing(pCommandList);
	UpdateShaderBindingTable();

	PIXBeginEvent(pCommandList, PIX_COLOR_DEFAULT, L"Render");

	pCommandList->SetComputeRootSignature(m_pGlobalRootSignature.Get());
	pCommandList->SetPipelineState1(m_pRaytracingStateObject.Get());

	ID3D12DescriptorHeap* heaps[] = { m_pDescriptorHeap_SRVUAV->Heap() };
	pCommandList->SetDescriptorHeaps(1, heaps);


	pCommandList->SetComputeRootShaderResourceView((UINT)GlobalRootSig::eSceneAS,
	//	m_pModelManager->m_pTopLevelAccelerationStructures[currentFrame]->GetGPUVirtualAddress());
		m_pModelManager->m_pTopLevelAccelerationStructures->GetGPUVirtualAddress());

	pCommandList->SetComputeRootConstantBufferView((UINT)GlobalRootSig::eSceneConstant,
		m_pPassConstant->GetPassConstantGpuAddress());


	D3D12_DISPATCH_RAYS_DESC dispatchRays{};
	dispatchRays.RayGenerationShaderRecord.StartAddress = m_pRaygenerationSBT->GetGPUVirtualAddress();
	dispatchRays.RayGenerationShaderRecord.SizeInBytes = sizeof(ShaderParameter);

	dispatchRays.HitGroupTable.StartAddress = m_pHitSBT.GpuAddress();
	dispatchRays.HitGroupTable.SizeInBytes = sizeof(ShaderParameter[2]);
	dispatchRays.HitGroupTable.StrideInBytes = sizeof(ShaderParameter);

	dispatchRays.MissShaderTable.StartAddress = m_pMissSBT.GpuAddress();
	dispatchRays.MissShaderTable.SizeInBytes = sizeof(ShaderParameter[1]);
	dispatchRays.MissShaderTable.StrideInBytes = sizeof(ShaderParameter);

	dispatchRays.Width = size.right - size.left;
	dispatchRays.Height = size.bottom - size.top;
	dispatchRays.Depth = 1;

	pCommandList->DispatchRays(&dispatchRays);

	PIXEndEvent(pCommandList);
}

void HRaytracing::ClearBuffer(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
	auto pCommandList = m_pDeviceResources->GetCommandList();

	ID3D12DescriptorHeap* heaps[] = { m_pDescriptorHeap_SRVUAV->Heap() };
	pCommandList->SetDescriptorHeaps(1, heaps);

	//float value[4] = { 0,0,0,0 };
	float value[4] = { 0.5f,0.5f,0.5f,1.f };

	pCommandList->ClearUnorderedAccessViewFloat(
		m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::OutputBuffer),
		cpuHandle,
		pResource, value, 0, NULL);
}

void HRaytracing::CreateGlobalRootSignature()
{
	HRESULT hr = S_OK;

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)GlobalRootSig::TotalCount];
	rootParameters[(UINT)GlobalRootSig::eSceneConstant].InitAsConstantBufferView(0);
	rootParameters[(UINT)GlobalRootSig::eSceneAS].InitAsShaderResourceView(0);
	
	auto staticSamplers = d3dUtil::GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(ARRAYSIZE(rootParameters), rootParameters
		, (UINT)staticSamplers.size(), staticSamplers.data());

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
		IID_PPV_ARGS(m_pGlobalRootSignature.GetAddressOf())));

	/* 
	HRESULT hr = S_OK;

	D3D12_DESCRIPTOR_RANGE renderTarget[]
	{
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV,		//range type
		1,										//numDecriptors
		0,										//BaseShaderRegister
		0,										//RegisterSpace
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND	//OffsetInDescriptorFromTableStart
	};

	CD3DX12_DESCRIPTOR_RANGE materialTable;
	materialTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_MATERIAL_DESCRIPTORHEAP_SIZE + 1, 0, 2);

	CD3DX12_DESCRIPTOR_RANGE gBuffer;
	gBuffer.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 3);

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)RootSig_Raytracing::MaxCount];
	rootParameters[(UINT)RootSig_Raytracing::OutputBuffer].InitAsDescriptorTable(1, renderTarget, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSig_Raytracing::RaytracingAccelerationStructure].InitAsShaderResourceView(0, 0);
	rootParameters[(UINT)RootSig_Raytracing::PassConstant].InitAsConstantBufferView(0);
	rootParameters[(UINT)RootSig_Raytracing::IndexBuffer].InitAsShaderResourceView(0, 1);
	rootParameters[(UINT)RootSig_Raytracing::VertexBuffer].InitAsShaderResourceView(1, 1);
	rootParameters[(UINT)RootSig_Raytracing::VertexBuffer_Dynamic].InitAsShaderResourceView(4, 1);
	rootParameters[(UINT)RootSig_Raytracing::structuredBuffer_PerInstance].InitAsShaderResourceView(2, 1);
	rootParameters[(UINT)RootSig_Raytracing::structuredBuffer_PerModel].InitAsShaderResourceView(3, 1);
	rootParameters[(UINT)RootSig_Raytracing::Material].InitAsDescriptorTable(1, &materialTable, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSig_Raytracing::GBuffer].InitAsDescriptorTable(1, &gBuffer, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[(UINT)RootSig_Raytracing::Light].InitAsShaderResourceView(5,1);

	auto staticSamplers = d3dUtil::GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(ARRAYSIZE(rootParameters), rootParameters
		, (UINT)staticSamplers.size(), staticSamplers.data());

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
		IID_PPV_ARGS(m_pRaytracingGlobalRootSignature.GetAddressOf())));
		*/

}

void HRaytracing::CreateLocalRootSignature_Raygen()
{
	HRESULT hr = S_OK;
	CD3DX12_DESCRIPTOR_RANGE outputBuffer_descriptorRange;
	outputBuffer_descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE normalBuffer_descriptorRange;
	normalBuffer_descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);

	CD3DX12_DESCRIPTOR_RANGE depthBuffer_descriptorRange;
	depthBuffer_descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 1);

	CD3DX12_DESCRIPTOR_RANGE metallicRoughnessAoEmissive_descriptorRange;
	metallicRoughnessAoEmissive_descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 1);

	CD3DX12_DESCRIPTOR_RANGE random_descriptorRange;
	random_descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 1);

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)LocalRootSig_Raygen::eTotalCount];
	rootParameters[(UINT)LocalRootSig_Raygen::eOutputBuffer]
		.InitAsDescriptorTable(1, &outputBuffer_descriptorRange);
	rootParameters[(UINT)LocalRootSig_Raygen::eNormalBuffer]
		.InitAsDescriptorTable(1, &normalBuffer_descriptorRange);
	rootParameters[(UINT)LocalRootSig_Raygen::eDepthBuffer]
		.InitAsDescriptorTable(1, &depthBuffer_descriptorRange);
	rootParameters[(UINT)LocalRootSig_Raygen::eMetallicRoughnessAoEmissive]
		.InitAsDescriptorTable(1, &metallicRoughnessAoEmissive_descriptorRange);
	rootParameters[(UINT)LocalRootSig_Raygen::eRandomTexture]
		.InitAsDescriptorTable(1, &random_descriptorRange);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(ARRAYSIZE(rootParameters), rootParameters);
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
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
		IID_PPV_ARGS(m_pLocalRootSig_Raygen.GetAddressOf())));
}

void HRaytracing::CreateLocalRootSignature_Miss()
{
	HRESULT hr = S_OK;
	CD3DX12_DESCRIPTOR_RANGE skybox_descriptorRange;
	skybox_descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)LocalRootSig_Miss::eTotalCount];
	rootParameters[(UINT)LocalRootSig_Miss::eSkyBox]
		.InitAsDescriptorTable(1, &skybox_descriptorRange);
	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(ARRAYSIZE(rootParameters), rootParameters);
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
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
		IID_PPV_ARGS(m_pLocalRootSig_Miss.GetAddressOf())));
}

void HRaytracing::CreateLocalRootSignature_HitGroup0()
{
	HRESULT hr = S_OK;

	CD3DX12_DESCRIPTOR_RANGE materialTextures;
	materialTextures.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_MATERIAL_DESCRIPTORHEAP_SIZE, 0, 2);

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)LocalRootSig_Hit0::eTotalCount];
	rootParameters[(UINT)LocalRootSig_Hit0::eStaticVertexBuffer].InitAsShaderResourceView(4, 1);
	rootParameters[(UINT)LocalRootSig_Hit0::eDynamicVertexBuffer].InitAsShaderResourceView(0, 1);
	rootParameters[(UINT)LocalRootSig_Hit0::eIndexBuffer].InitAsShaderResourceView(3, 1);
	rootParameters[(UINT)LocalRootSig_Hit0::eLight].InitAsShaderResourceView(2, 1);
	rootParameters[(UINT)LocalRootSig_Hit0::eInstanceData].InitAsShaderResourceView(1, 1);
	rootParameters[(UINT)LocalRootSig_Hit0::eModelData].InitAsShaderResourceView(5, 1);
	rootParameters[(UINT)LocalRootSig_Hit0::eMaterialTextures].InitAsDescriptorTable(1, &materialTextures);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(ARRAYSIZE(rootParameters), rootParameters);
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
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
		IID_PPV_ARGS(m_pLocalRootSig_HitGroup0.GetAddressOf())));
}

void HRaytracing::CreateLocalRootSignature_HitGroup1()
{
	HRESULT hr = S_OK;

	CD3DX12_DESCRIPTOR_RANGE colorChipTextures;
	colorChipTextures.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 2);

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)LocalRootSig_Hit1::eTotalCount];
	rootParameters[(UINT)LocalRootSig_Hit1::eStaticVertexBuffer].InitAsShaderResourceView(4, 1);
	rootParameters[(UINT)LocalRootSig_Hit1::eDynamicVertexBuffer].InitAsShaderResourceView(0, 1);
	rootParameters[(UINT)LocalRootSig_Hit1::eIndexBuffer].InitAsShaderResourceView(3, 1);
	rootParameters[(UINT)LocalRootSig_Hit1::eLight].InitAsShaderResourceView(2, 1);
	rootParameters[(UINT)LocalRootSig_Hit1::eInstanceData].InitAsShaderResourceView(1, 1);
	rootParameters[(UINT)LocalRootSig_Hit1::eModelData].InitAsShaderResourceView(5, 1);
	rootParameters[(UINT)LocalRootSig_Hit1::eColorChipTexture].InitAsDescriptorTable(1, &colorChipTextures);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(ARRAYSIZE(rootParameters), rootParameters);
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
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
		IID_PPV_ARGS(m_pLocalRootSig_HitGroup1.GetAddressOf())));
}

void HRaytracing::CreateRaytracingStateObject()
{
	enum subobjectIndex
	{
		eLibrary_raygen,
		eLibrary_miss,
		eLibrary_hit0,
		eLibrary_hit1,
		eHitGroup0,
		eHitGroup1,
		eShaderConfig,
		ePayloadAssociation,
		eLocalRootSig_raygen,
		eRootSignatureAssociation_raygen,
		eLocalRootSig_miss,
		eRootSignatureAssociation_miss,
		eLocalRootSig_hitGroup0,
		eRootSignatureAssociation_hitGroup0,
		eLocalRootSig_hitGroup1,
		eRootSignatureAssociation_hitGroup1,
		eGlobalRootSig,
		ePipelineConfig,
		eTotal

	};

	D3D12_STATE_SUBOBJECT subobjects[eTotal];

	D3D12_EXPORT_DESC exportDesc_raygen 
		= { L"RayGeneration", nullptr, D3D12_EXPORT_FLAG_NONE };

	D3D12_DXIL_LIBRARY_DESC libraryDesc_raygen{};
	libraryDesc_raygen =
	{
		{g_pRayGeneration, sizeof(g_pRayGeneration)},		
		1,													
		&exportDesc_raygen									
	};

	subobjects[eLibrary_raygen].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[eLibrary_raygen].pDesc = &libraryDesc_raygen;

	D3D12_EXPORT_DESC exportDesc_miss
		= { L"Miss", nullptr, D3D12_EXPORT_FLAG_NONE };

	D3D12_DXIL_LIBRARY_DESC libraryDesc_miss{};
	libraryDesc_miss =
	{
		{g_pMiss, sizeof(g_pMiss)},
		1,
		&exportDesc_miss
	};

	subobjects[eLibrary_miss].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[eLibrary_miss].pDesc = &libraryDesc_miss;

	D3D12_EXPORT_DESC exportDesc_hit0
		= { L"Hit0", nullptr, D3D12_EXPORT_FLAG_NONE };

	D3D12_DXIL_LIBRARY_DESC libraryDesc_hit0{};
	libraryDesc_hit0 =
	{
		{g_pHitGroup0, sizeof(g_pHitGroup0)},
		1,
		&exportDesc_hit0
	};

	subobjects[eLibrary_hit0].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[eLibrary_hit0].pDesc = &libraryDesc_hit0;

	D3D12_EXPORT_DESC exportDesc_hit1
		= { L"Hit1", nullptr, D3D12_EXPORT_FLAG_NONE };

	D3D12_DXIL_LIBRARY_DESC libraryDesc_hit1{};
	libraryDesc_hit1 =
	{
		{g_pHitGroup1, sizeof(g_pHitGroup1)},
		1,
		&exportDesc_hit1
	};

	subobjects[eLibrary_hit1].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[eLibrary_hit1].pDesc = &libraryDesc_hit1;


	D3D12_HIT_GROUP_DESC hitGroupDesc_hitGroup0
	{
		L"HitGroup0",					//HitGrouptExport
		D3D12_HIT_GROUP_TYPE_TRIANGLES,	//Type
		nullptr,						//AnyHitShaderImport
		L"Hit0",						//ClosestHitShaderImport
		nullptr							//IntersectionShaderImport
	};

	subobjects[eHitGroup0].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subobjects[eHitGroup0].pDesc = &hitGroupDesc_hitGroup0;

	D3D12_HIT_GROUP_DESC hitGroupDesc_hitGroup1
	{
		L"HitGroup1",					//HitGrouptExport
		D3D12_HIT_GROUP_TYPE_TRIANGLES,	//Type
		nullptr,						//AnyHitShaderImport
		L"Hit1",						//ClosestHitShaderImport
		nullptr							//IntersectionShaderImport
	};

	subobjects[eHitGroup1].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subobjects[eHitGroup1].pDesc = &hitGroupDesc_hitGroup1;

	//레이트레이싱 쉐이더의 기초 정보를 설정
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig
	{
		sizeof(float[6]),									//MaxPayLoadSizeInBytes
															//레이트레이싱 추적 과정 중 각 쉐이더
															//코드 사이를 이동하는 정보 집합
		sizeof(float[2])									//MaxAttributeSizeInBytes
															//추적 과정 중 대상 삼각형에 Hit했을 경우
															//삼각형에 대한 정보(Attribute)를 생성하는 것의 
															//최대 크기
	};

	subobjects[eShaderConfig].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subobjects[eShaderConfig].pDesc = &shaderConfig;

	std::vector<LPCWCHAR> exportedSymbols;
	exportedSymbols.push_back(L"RayGeneration");
	exportedSymbols.push_back(L"Miss");
	exportedSymbols.push_back(L"HitGroup0");
	exportedSymbols.push_back(L"HitGroup1");


	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
	shaderPayloadAssociation.NumExports = static_cast<UINT>(exportedSymbols.size());
	shaderPayloadAssociation.pExports = exportedSymbols.data();
	shaderPayloadAssociation.pSubobjectToAssociate = &subobjects[eShaderConfig];

	subobjects[ePayloadAssociation].Type =
		D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobjects[ePayloadAssociation].pDesc = &shaderPayloadAssociation;

	subobjects[eLocalRootSig_raygen].Type 
		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subobjects[eLocalRootSig_raygen].pDesc
		= m_pLocalRootSig_Raygen.GetAddressOf();

	LPCWCHAR symbol0 = L"RayGeneration";
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSigAssociation_raygen = {};
	localRootSigAssociation_raygen.pExports = &symbol0;
	localRootSigAssociation_raygen.NumExports = 1;
	localRootSigAssociation_raygen.pSubobjectToAssociate 
		= &subobjects[eLocalRootSig_raygen];

	subobjects[eRootSignatureAssociation_raygen].Type =
		D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobjects[eRootSignatureAssociation_raygen].pDesc =
		&localRootSigAssociation_raygen;

	subobjects[eLocalRootSig_miss].Type
		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subobjects[eLocalRootSig_miss].pDesc
		= m_pLocalRootSig_Miss.GetAddressOf();

	LPCWCHAR symbol1 = L"Miss";
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSigAssociation_miss = {};
	localRootSigAssociation_miss.pExports = &symbol1;
	localRootSigAssociation_miss.NumExports = 1;
	localRootSigAssociation_miss.pSubobjectToAssociate
		= &subobjects[eLocalRootSig_miss];

	subobjects[eRootSignatureAssociation_miss].Type =
		D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobjects[eRootSignatureAssociation_miss].pDesc =
		&localRootSigAssociation_miss;

	subobjects[eLocalRootSig_hitGroup0].Type
		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subobjects[eLocalRootSig_hitGroup0].pDesc
		= m_pLocalRootSig_HitGroup0.GetAddressOf();

	LPCWCHAR symbol2 = L"HitGroup0";
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSigAssociation_hitGroup0 = {};
	localRootSigAssociation_hitGroup0.pExports = &symbol2;
	localRootSigAssociation_hitGroup0.NumExports = 1;
	localRootSigAssociation_hitGroup0.pSubobjectToAssociate
		= &subobjects[eLocalRootSig_hitGroup0];

	subobjects[eRootSignatureAssociation_hitGroup0].Type =
		D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobjects[eRootSignatureAssociation_hitGroup0].pDesc =
		&localRootSigAssociation_hitGroup0;

	subobjects[eLocalRootSig_hitGroup1].Type
		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subobjects[eLocalRootSig_hitGroup1].pDesc
		= m_pLocalRootSig_HitGroup1.GetAddressOf();

	LPCWCHAR symbol3 = L"HitGroup1";
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSigAssociation_hitGroup1 = {};
	localRootSigAssociation_hitGroup1.pExports = &symbol3;
	localRootSigAssociation_hitGroup1.NumExports = 1;
	localRootSigAssociation_hitGroup1.pSubobjectToAssociate
		= &subobjects[eLocalRootSig_hitGroup1];

	subobjects[eRootSignatureAssociation_hitGroup1].Type =
		D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	subobjects[eRootSignatureAssociation_hitGroup1].pDesc =
		&localRootSigAssociation_hitGroup1;

	subobjects[eGlobalRootSig].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subobjects[eGlobalRootSig].pDesc = m_pGlobalRootSignature.GetAddressOf();

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig
	{
		3													//MaxTraceRecursionDepth
															//물체와의 Hit 판정을 위해 광선(Ray)를
															//함수(TraceRay)를 통해 발사할 텐데, 이 함수를
															//몇 번 재귀적으로 사용할 수 있는 지 결정
	};

	subobjects[ePipelineConfig].Type = 
		D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subobjects[ePipelineConfig].pDesc =
		&pipelineConfig;

	D3D12_STATE_OBJECT_DESC raytracingStateObject
	{
		D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		ARRAYSIZE(subobjects),
		subobjects
	};

	DX::ThrowIfFailed(m_pDeviceResources->GetRaytracingDevice()->CreateStateObject(&raytracingStateObject,
		IID_PPV_ARGS(&m_pRaytracingStateObject)));

	DX::ThrowIfFailed(m_pRaytracingStateObject->QueryInterface(
		IID_PPV_ARGS(&m_pRaytracingStateObjectProperties)));

	/*
		D3D12_EXPORT_DESC rayGenerationExport[3]
	{
		{L"RayGeneration", nullptr, D3D12_EXPORT_FLAG_NONE},//함수이름
		{L"Hit", nullptr, D3D12_EXPORT_FLAG_NONE},//함수이름
		{L"Miss", nullptr, D3D12_EXPORT_FLAG_NONE},//함수이름
	};

	D3D12_DXIL_LIBRARY_DESC rayGeneration{};
	rayGeneration =
	{
		{g_pRaytracing, sizeof(g_pRaytracing)},			//DXILLibrary
		3,												//ExportCount
		rayGenerationExport								//ExportArray
	};

	D3D12_STATE_SUBOBJECT shaders
	{
		D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,			//type
		&rayGeneration										//pDesc
	};

	//레이트레이싱 쉐이더의 기초 정보를 설정
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig
	{
		sizeof(float[6]),									//MaxPayLoadSizeInBytes
															//레이트레이싱 추적 과정 중 각 쉐이더
															//코드 사이를 이동하는 정보 집합
		sizeof(float[2])									//MaxAttributeSizeInBytes
															//추적 과정 중 대상 삼각형에 Hit했을 경우
															//삼각형에 대한 정보(Attribute)를 생성하는 것의 
															//최대 크기
	};

	D3D12_STATE_SUBOBJECT config
	{
		D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,//type
		&shaderConfig										//pDesc
	};

	D3D12_STATE_SUBOBJECT globalRS
	{
		D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,	//type
		m_pRaytracingGlobalRootSignature.GetAddressOf()		//ID3D12RootSignature
	};

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig
	{
		3													//MaxTraceRecursionDepth
															//물체와의 Hit 판정을 위해 광선(Ray)를
															//함수(TraceRay)를 통해 발사할 텐데, 이 함수를
															//몇 번 재귀적으로 사용할 수 있는 지 결정
	};

	D3D12_STATE_SUBOBJECT pipeline
	{
		D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,//type
		&pipelineConfig										  //pDesc
	};

	D3D12_HIT_GROUP_DESC hitGroup
	{
		L"HitGroup0",					//HitGrouptExport
		D3D12_HIT_GROUP_TYPE_TRIANGLES,	//Type
		nullptr,						//AnyHitShaderImport
		L"Hit",							//ClosestHitShaderImport
		nullptr							//IntersectionShaderImport
	};

	D3D12_STATE_SUBOBJECT hitGroupObject
	{
		D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
		&hitGroup
	};

	D3D12_STATE_SUBOBJECT subobjects[]
	{
		shaders,
		hitGroupObject,
		config,
		globalRS,
		pipeline
	};

	D3D12_STATE_OBJECT_DESC raytracingStateObject
	{
		D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		ARRAYSIZE(subobjects),
		subobjects
	};

	DX::ThrowIfFailed(m_pDeviceResources->GetRaytracingDevice()->CreateStateObject(&raytracingStateObject,
		IID_PPV_ARGS(&m_pRaytracingStateObject)));

	DX::ThrowIfFailed(m_pRaytracingStateObject->QueryInterface(
		IID_PPV_ARGS(&m_pRaytracingStateObjectProperties)));
	*/

}

void HRaytracing::InitializeShaderBindingTable()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	ResourceUploadBatch batch(device);
	batch.Begin();

	ShaderParameter parameter;

	memcpy(
		parameter.ShaderIdentifier, m_pRaytracingStateObjectProperties->GetShaderIdentifier(L"RayGeneration"),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
	);

	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
		m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::OutputBuffer);
	UINT64* pheapPtr = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

	memcpy(&parameter.RootParameters[(UINT)LocalRootSig_Raygen::eOutputBuffer], &pheapPtr, sizeof(UINT64));

	srvUavHeapHandle =
		m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::Normal);
	pheapPtr = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

	memcpy(&parameter.RootParameters[(UINT)LocalRootSig_Raygen::eNormalBuffer], &pheapPtr, sizeof(UINT64));

	srvUavHeapHandle =
		m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::DepthMap);
	pheapPtr = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

	memcpy(&parameter.RootParameters[(UINT)LocalRootSig_Raygen::eDepthBuffer], &pheapPtr, sizeof(UINT64));

	srvUavHeapHandle =
		m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::MetallicRoughnessAoEmissive);
	pheapPtr = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

	memcpy(&parameter.RootParameters[(UINT)LocalRootSig_Raygen::eMetallicRoughnessAoEmissive],
		&pheapPtr, sizeof(UINT64));

	srvUavHeapHandle =
		m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::RandomTexture);
	pheapPtr = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

	memcpy(&parameter.RootParameters[(UINT)LocalRootSig_Raygen::eRandomTexture],
		&pheapPtr, sizeof(UINT64));

	DirectX::CreateStaticBuffer(device, batch, &parameter, 1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		&m_pRaygenerationSBT);

	auto uploadResourcesFinished = batch.End(m_pDeviceResources->GetCommandQueue());

	//GPU가 모든 명령어를 실행하기를 기다린다.
	uploadResourcesFinished.wait();

}

void HRaytracing::UpdateShaderBindingTable()
{
	ShaderParameter parameter;
	{
		memcpy(
			parameter.ShaderIdentifier, m_pRaytracingStateObjectProperties->GetShaderIdentifier(L"Miss"),
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
		);

		D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
			m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::Skybox);
		UINT64* pheapPtr = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

		memcpy(&parameter.RootParameters[(UINT)LocalRootSig_Miss::eSkyBox], &pheapPtr, sizeof(UINT64));

		UINT bufferSize = sizeof(ShaderParameter);
		SharedGraphicsResource graphicResourceCB =
			m_pGraphicsMemory->AllocateConstant(bufferSize);
		memcpy(graphicResourceCB.Memory(),
			&parameter,
			bufferSize);

		m_pMissSBT.Reset(graphicResourceCB);
	}

	ShaderParameter parameter1 = {};
	ShaderParameter parameter2 = {};
	{
		
		memcpy(
			parameter1.ShaderIdentifier, m_pRaytracingStateObjectProperties->GetShaderIdentifier(L"HitGroup0"),
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
		);

		parameter1.RootParameters[(UINT)LocalRootSig_Hit0::eStaticVertexBuffer] =
			m_pModelManager->m_allVerticesResource->GetGPUVirtualAddress();
		if(m_pModelManager->m_pAllInstacedVertices)
		parameter1.RootParameters[(UINT)LocalRootSig_Hit0::eDynamicVertexBuffer] =
			m_pModelManager->m_pAllInstacedVertices->GetGPUVirtualAddress();
		parameter1.RootParameters[(UINT)LocalRootSig_Hit0::eIndexBuffer] =
			m_pModelManager->m_allIndicesResource->GetGPUVirtualAddress();
		parameter1.RootParameters[(UINT)LocalRootSig_Hit0::eLight] =
			m_pPassConstant->GetLightGpyAddress();
		parameter1.RootParameters[(UINT)LocalRootSig_Hit0::eInstanceData] =
			m_pModelManager->m_raytracingStructuredBuffer_perInstance.GpuAddress();
		parameter1.RootParameters[(UINT)LocalRootSig_Hit0::eModelData] =
			m_pModelManager->m_raytracingStructuredBuffer_perModel.GpuAddress();

		D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
		m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::Material);
		UINT64* pheapPtr = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

		parameter1.RootParameters[(UINT)LocalRootSig_Hit0::eMaterialTextures] =
			(UINT64)pheapPtr;
	}

	{
		memcpy(
			parameter2.ShaderIdentifier, m_pRaytracingStateObjectProperties->GetShaderIdentifier(L"HitGroup1"),
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
		);

		parameter2.RootParameters[(UINT)LocalRootSig_Hit1::eStaticVertexBuffer] =
			m_pModelManager->m_allVerticesResource->GetGPUVirtualAddress();
		if (m_pModelManager->m_pAllInstacedVertices)
		parameter2.RootParameters[(UINT)LocalRootSig_Hit1::eDynamicVertexBuffer] =
			m_pModelManager->m_pAllInstacedVertices->GetGPUVirtualAddress();
		parameter2.RootParameters[(UINT)LocalRootSig_Hit1::eIndexBuffer] =
			m_pModelManager->m_allIndicesResource->GetGPUVirtualAddress();
		parameter2.RootParameters[(UINT)LocalRootSig_Hit1::eLight] =
			m_pPassConstant->GetLightGpyAddress();
		parameter2.RootParameters[(UINT)LocalRootSig_Hit1::eInstanceData] =
			m_pModelManager->m_raytracingStructuredBuffer_perInstance.GpuAddress();
		parameter2.RootParameters[(UINT)LocalRootSig_Hit1::eModelData] =
			m_pModelManager->m_raytracingStructuredBuffer_perModel.GpuAddress();

		D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
			m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::ColorChip);
		UINT64* pheapPtr = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

		parameter2.RootParameters[(UINT)LocalRootSig_Hit1::eColorChipTexture] =
			(UINT64)pheapPtr;
	}

	//ShaderParameter hitGroupSBTs[] = { };
	ShaderParameter hitGroupSBTs[] = { parameter1, parameter2 };

	UINT bufferSize = sizeof(ShaderParameter) * ARRAYSIZE(hitGroupSBTs);
	SharedGraphicsResource graphicResourceCB =
		m_pGraphicsMemory->AllocateConstant(hitGroupSBTs);
	memcpy(graphicResourceCB.Memory(),
		hitGroupSBTs,
		bufferSize);

	m_pHitSBT.Reset(graphicResourceCB);
}
