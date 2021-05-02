#include"pch.h"
#include"HRaytracing.h"
#include"DeviceResources.h"
#include"HTextureManager.h"
#include"HModelManager.h"
#include"HPassConstant.h"
#include"Raytracing.hlsl.h"
#include"d3dUtil.h"



//#include"SkinAnim_Raytracing.hlsl.h"

void HRaytracing::Initialize(DeviceResources* pDeviceResources, HModelManager* pModelManager, HPassConstant* pPassConstant, HTextureManager* pTextureManager)
{
	m_pDeviceResources = pDeviceResources;
	m_pModelManager = pModelManager;
	m_pPassConstant = pPassConstant;
	m_pTextureManager = pTextureManager;
}

void HRaytracing::CreateDeviceDependentResources(ResourceUploadBatch& resourceBatch)
{
	CreateRaytracingRootSignature();
	CreateRaytracingStateObject();
	CreateShaderBindingTable(resourceBatch);

	m_pDescriptorHeap_SRVUAV =
		std::make_unique<DescriptorHeap>(m_pDeviceResources->GetD3DDevice(), (UINT)SRVUAVDescriptorHeapOffset_Raytracing::MaxCount);
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

void HRaytracing::CreateDescriptors(ID3D12Resource* pReflection, ID3D12Resource* pNormal, ID3D12Resource* pDepth)
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

	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	device->CreateShaderResourceView(
		pDepth, &srvDesc,
		m_pDescriptorHeap_SRVUAV->GetCpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::DepthMap));

	

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

		m_pTextureManager->SetRaytracingDirty(false);
	}
}

void HRaytracing::Draw()
{
	if (m_pModelManager->m_models.size() == 0)
		return;

	auto pCommandList = m_pDeviceResources->GetRaytracingCommandList();
	UINT currentFrame = m_pDeviceResources->GetCurrentFrameIndex();
	auto size = m_pDeviceResources->GetOutputSize();
	auto backBuffer = m_pDeviceResources->GetRenderTarget();

	m_pModelManager->UpdateVertexBuffer_raytracing(pCommandList);
	m_pModelManager->UpdateRaytracingBottomLevelAccelerationStructure_raytracing(pCommandList);
	m_pModelManager->UpdateRaytracingTopLevelAccelerationStructure_raytracing(pCommandList);

	PIXBeginEvent(pCommandList, PIX_COLOR_DEFAULT, L"Render");

	pCommandList->SetComputeRootSignature(m_pRaytracingGlobalRootSignature.Get());
	pCommandList->SetPipelineState1(m_pRaytracingStateObject.Get());

	ID3D12DescriptorHeap* heaps[] = { m_pDescriptorHeap_SRVUAV->Heap() };
	pCommandList->SetDescriptorHeaps(1, heaps);

	pCommandList->SetComputeRootDescriptorTable((UINT)RootSig_Raytracing::OutputBuffer,
		m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::OutputBuffer));
	pCommandList->SetComputeRootShaderResourceView((UINT)RootSig_Raytracing::RaytracingAccelerationStructure,
		m_pModelManager->m_pTopLevelAccelerationStructures[currentFrame]->GetGPUVirtualAddress());
	pCommandList->SetComputeRootConstantBufferView((UINT)RootSig_Raytracing::PassConstant,
		m_pPassConstant->GetPassConstantGpuAddress());
	pCommandList->SetComputeRootShaderResourceView((UINT)RootSig_Raytracing::IndexBuffer,
		m_pModelManager->m_allIndicesResource->GetGPUVirtualAddress());
	pCommandList->SetComputeRootShaderResourceView((UINT)RootSig_Raytracing::VertexBuffer,
		m_pModelManager->m_allVerticesResource->GetGPUVirtualAddress());
	if(m_pModelManager->m_pAllInstacedVertices.Get()!=nullptr)
	pCommandList->SetComputeRootShaderResourceView((UINT)RootSig_Raytracing::VertexBuffer_Dynamic,
		m_pModelManager->m_pAllInstacedVertices->GetGPUVirtualAddress());
	if(m_pModelManager->m_raytracingStructuredBuffer_perInstance)
	pCommandList->SetComputeRootShaderResourceView((UINT)RootSig_Raytracing::structuredBuffer_PerInstance,
		m_pModelManager->m_raytracingStructuredBuffer_perInstance.GpuAddress());
	if (m_pModelManager->m_raytracingStructuredBuffer_perModel)
	pCommandList->SetComputeRootShaderResourceView((UINT)RootSig_Raytracing::structuredBuffer_PerModel,
		m_pModelManager->m_raytracingStructuredBuffer_perModel.GpuAddress());

	pCommandList->SetComputeRootDescriptorTable((UINT)RootSig_Raytracing::Material,
		m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::Material));

	pCommandList->SetComputeRootDescriptorTable((UINT)RootSig_Raytracing::GBuffer,
		m_pDescriptorHeap_SRVUAV->GetGpuHandle((UINT)SRVUAVDescriptorHeapOffset_Raytracing::Normal));

	pCommandList->SetComputeRootShaderResourceView((UINT)RootSig_Raytracing::Light,
		m_pPassConstant->GetLightGpyAddress());

	D3D12_DISPATCH_RAYS_DESC dispatchRays{};
	dispatchRays.RayGenerationShaderRecord.StartAddress = m_pRaygenerationSBT->GetGPUVirtualAddress();
	dispatchRays.RayGenerationShaderRecord.SizeInBytes = sizeof(ShaderParameter);

	dispatchRays.HitGroupTable.StartAddress = m_pHitSBT->GetGPUVirtualAddress();
	dispatchRays.HitGroupTable.SizeInBytes = sizeof(ShaderParameter[1]);
	dispatchRays.HitGroupTable.StrideInBytes = sizeof(ShaderParameter);

	dispatchRays.MissShaderTable.StartAddress = m_pMissSBT->GetGPUVirtualAddress();
	dispatchRays.MissShaderTable.SizeInBytes = sizeof(ShaderParameter[1]);
	dispatchRays.MissShaderTable.StrideInBytes = sizeof(ShaderParameter);

	dispatchRays.Width = size.right - size.left;
	dispatchRays.Height = size.bottom - size.top;
	dispatchRays.Depth = 1;

	pCommandList->DispatchRays(&dispatchRays);

	PIXEndEvent(pCommandList);
	/*PIXBeginEvent(pCommandList, PIX_COLOR_DEFAULT, L"Copy");

	D3D12_RESOURCE_BARRIER barriers[2];
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_raytracingOutput.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffer,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_COPY_DEST);

	pCommandList->ResourceBarrier(2, barriers);

	pCommandList->CopyResource(backBuffer, m_raytracingOutput.Get());

	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_raytracingOutput.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	pCommandList->ResourceBarrier(2, barriers);

	PIXEndEvent(pCommandList);*/
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

void HRaytracing::CreateRaytracingRootSignature()
{
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

	/*CD3DX12_DESCRIPTOR_RANGE skyboxTable;
	materialTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SKYBOXMAP_COUNT, 0, 2);*/

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


}

void HRaytracing::CreateRaytracingStateObject()
{
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
}

void HRaytracing::CreateShaderBindingTable(ResourceUploadBatch& resourceBatch)
{
	auto device = m_pDeviceResources->GetD3DDevice();

	ShaderParameter parameter;

	memcpy(
		parameter.ShaderIdentifier, m_pRaytracingStateObjectProperties->GetShaderIdentifier(L"RayGeneration"),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
	);

	DirectX::CreateStaticBuffer(device, resourceBatch, &parameter, 1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		&m_pRaygenerationSBT);

	memcpy(
		parameter.ShaderIdentifier, m_pRaytracingStateObjectProperties->GetShaderIdentifier(L"HitGroup0"),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
	);

	DirectX::CreateStaticBuffer(device, resourceBatch, &parameter, 1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		&m_pHitSBT);

	memcpy(
		parameter.ShaderIdentifier, m_pRaytracingStateObjectProperties->GetShaderIdentifier(L"Miss"),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
	);

	DirectX::CreateStaticBuffer(device, resourceBatch, &parameter, 1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		&m_pMissSBT);

}
