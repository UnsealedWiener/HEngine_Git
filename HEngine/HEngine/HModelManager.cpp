#include"pch.h"
#include"HModelManager.h"
#include"DeviceResources.h"
#include"HFBXImporter.h"
#include"HSelfFOrmatReader.h"
#include"HBufferManager.h"
#include"SkinAnim_Raytracing.hlsl.h"

HModelManager* HModelManager::GetInstance()
{
	static HModelManager modelManager;
	return &modelManager;
}

void HModelManager::Initialize(DeviceResources* pDeviceResources, GraphicsMemory* pGraphicMemory, Camera* pCamera)
{
	m_pDeviceResources = pDeviceResources;
	m_pGraphicMemory = pGraphicMemory;
	m_pCamera = pCamera;

	m_pTopLevelAccelerationStructures.resize(m_pDeviceResources->GetBackBufferCount());
	m_pTopLevelAccelerationStructureScratches.resize(m_pDeviceResources->GetBackBufferCount());
	m_pRaytracingInstanceDescs.resize(m_pDeviceResources->GetBackBufferCount());
	//m_pAllInstacedVertices.resize(m_pDeviceResources->GetBackBufferCount());

	CreateComputeRootSignature();
	CreateComputePSO_lightCalculation();
}

HModelData* HModelManager::CreateModelFromFbxFile(std::string fbxName, ResourceUploadBatch& resourceBatch)
{
	//CreateModelFromHModelFile("Media/Model/Crunch/Crunch_LOD4.hmodel", resourceBatch);

	if (fbxName.empty())
	{
		HerrorMessage(L"The fbxFiles name is empty.")
	}

	std::shared_ptr<HModel> pModel = std::make_shared<HModel>();

	std::unique_ptr<HFbxImporter> pFbxImporter = std::make_unique<HFbxImporter>();

	pFbxImporter->ImportModel(fbxName.c_str());

	CreateModelFromImportedFbx(pFbxImporter.get(), resourceBatch, pModel.get());

	pModel->pRawData = &pModel->rawData;
	pModel->managerController = HManagerController(this, &m_models, pModel.get());
	m_models[pModel.get()] = pModel;

	return pModel.get();
}

HAnimData* HModelManager::CreateAnimationFromFbxFiles(std::vector<std::string> fbxNames)
{
	if (fbxNames.empty())
	{
		HerrorMessage(L"The fbxFiles name is empty.")
	}

	std::shared_ptr<HAnim> pAnim = std::make_shared<HAnim>();
	

	for (int i = 0; i < fbxNames.size(); i++)
	{
		std::unique_ptr<HFbxImporter> pFbxImporter = std::make_unique<HFbxImporter>();
		pFbxImporter->ImportAnimation(fbxNames[i].c_str());
		CreateAnimationFromImportedFbx(pFbxImporter.get(), pAnim.get());
	}
	pAnim->pRawData = &pAnim->rawData;
	pAnim->managerController = HManagerController(this, &m_anims, pAnim.get());

	m_anims[pAnim.get()] = pAnim;

	return pAnim.get();
}

HModelData* HModelManager::CreateModelFromHModelFile(std::string fbxName, ResourceUploadBatch& resourceBatch)
{
	if (fbxName.empty())
	{
		HerrorMessage(L"The fbxFiles name is empty.")
	}

	HSelfFormatReader reader;
	HModelFormat* pHModel = reader.ReadHModelFile(fbxName.c_str());

	std::shared_ptr<HModel> pModel = std::make_shared<HModel>();

	CreateModelFromLoadedHModelFile(pHModel, pModel.get(), resourceBatch);

	pModel->pRawData = &pModel->rawData;
	pModel->managerController = HManagerController(this, &m_models, pModel.get());
	m_models[pModel.get()] = pModel;

	return pModel.get();
}

HAnimData* HModelManager::CreateAnimationFromHAnimFiles(std::vector<std::string> fbxNames)
{
	if (fbxNames.empty())
	{
		HerrorMessage(L"The fbxFiles name is empty.")
	}

	std::shared_ptr<HAnim> pAnim = std::make_shared<HAnim>();

	HSelfFormatReader reader;

	for (int i = 0; i < fbxNames.size(); i++)
	{
		HAnimFormat* pHAnimFormat = reader.ReadHAnimFile(fbxNames[i].c_str());
		std::unique_ptr<HBoneAnims> pHBoneAnims = std::make_unique<HBoneAnims>();
		CreateAnimDataFromLoadedHAnimFile(pHBoneAnims.get(), pHAnimFormat);
		pAnim->rawData.animBundle[pHAnimFormat->header.name] = std::move(pHBoneAnims);
	}

	pAnim->pRawData = &pAnim->rawData;
	pAnim->managerController = HManagerController(this, &m_anims, pAnim.get());

	m_anims[pAnim.get()] = pAnim;

	return pAnim.get();

}

void HModelManager::CreateAnimationFromImportedFbx(HFbxImporter* pFbxImporter, HAnim* pAnim)
{
	std::unique_ptr<HBoneAnims> pHAnim = std::make_unique<HBoneAnims>();
	CreateAnimDataFromImportedFbx(pHAnim.get(), &pFbxImporter->m_animData);
	pAnim->rawData.animBundle[pFbxImporter->m_animData.name] = std::move(pHAnim);
}

void HModelManager::CreateAnimDataFromImportedFbx(HBoneAnims* pHAnim, AnimationData* pFbxAnimData)
{
	pHAnim->totalTime = pFbxAnimData->totalTime;

	for (int i = 0; i < pFbxAnimData->allBoneAnim.size(); i++)
	{
		HBoneAnim pHBoneAnim;
		NodeAnimData* pCurrBone = pFbxAnimData->allBoneAnim[i];

		for (int j = 0; j < pCurrBone->keyFrames.size(); j++)
		{
			HAnimKey animKey;

			animKey.quatRot = pCurrBone->keyFrames[j].quatRot;
			animKey.scale = pCurrBone->keyFrames[j].scale;
			animKey.trans = pCurrBone->keyFrames[j].trans;
			//animKey.globalTM = pCurrBone->keyFrames[j].globalTM;
			animKey.time = pCurrBone->keyFrames[j].time;

			pHBoneAnim.animKeys.push_back(animKey);
		}
		pHBoneAnim.name = pCurrBone->nodeName;
		pHAnim->boneAnim.push_back(pHBoneAnim);
	}
}

void HModelManager::CreateAnimDataFromLoadedHAnimFile(HBoneAnims* pHAnim, HAnimFormat* pHAnimFormat)
{
	pHAnim->totalTime = pHAnimFormat->header.totalTime;

	for (int i = 0; i < pHAnimFormat->header.boneCount; i++)
	{
		HBoneAnim pHBoneAnim;
		HAnim_Bone* pSelfFormatAnimBone = &pHAnimFormat->pBone[i];
		HAnimKey* pHAnimKey = reinterpret_cast<HAnimKey*>(pSelfFormatAnimBone->pKey);
		pHBoneAnim.animKeys.assign(pHAnimKey, pHAnimKey + pSelfFormatAnimBone->keyCount);

		pHBoneAnim.name = pSelfFormatAnimBone->name;
		pHAnim->boneAnim.push_back(pHBoneAnim);
	}
}

void HModelManager::CreateModelFromLoadedHModelFile(HModelFormat* pHmodelFormat, HModel* pModel, ResourceUploadBatch& resourceBatch)
{
	HVertex* pHVertex = reinterpret_cast<HVertex*>(pHmodelFormat->pVertices);
	pModel->rawData.vertexData.assign(pHVertex, pHVertex + pHmodelFormat->header.totalVertexCount);

	pModel->rawData.indexData.assign(pHmodelFormat->pIndices,
		pHmodelFormat->pIndices + pHmodelFormat->header.totalIndexCount);

	pModel->rawData.AxisSystempModify = pHmodelFormat->commonValue.axisSystemModify;
	pModel->rawData.boundingBox = pHmodelFormat->commonValue.boundingBox;

	pModel->rawData.boneCount = pHmodelFormat->header.boneCount;

	XMMATRIX axisModify = XMLoadFloat4x4(&pModel->rawData.AxisSystempModify);
	pModel->rawData.boundingBox.Transform(pModel->rawData.boundingBox, axisModify);

	for (int i = 0; i < pHmodelFormat->header.meshCount; i++)
	{
		std::unique_ptr<HMesh> pHNode = std::make_unique<HMesh>();
		pHNode->name = pHmodelFormat->pMesh[i].name;

		if (pHmodelFormat->pMesh[i].bIsDepthZero)
		{
			pModel->rawData.pDepthZeroMeshNodes.push_back(pHNode.get());
		}

		pModel->rawData.pAllMeshNodes.push_back(std::move(pHNode));
	}

	for (int i = 0; i < pHmodelFormat->header.meshCount; i++)
	{
		if (pHmodelFormat->pMesh[i].childCount != 0)
		{
			for (int j = 0; j < pHmodelFormat->pMesh[i].childCount; j++)
			{
				pModel->rawData.pAllMeshNodes[i]->pChildNode.
					push_back(pModel->rawData.pAllMeshNodes[pHmodelFormat->pMesh[i].pChildIndex[j]].get());
			}
		}
	}

	for (int i = 0; i < pHmodelFormat->header.boneCount; i++)
	{
		std::unique_ptr<HBone> pHBone = std::make_unique<HBone>();
		
		pHBone->boneIndex = pHmodelFormat->pBone[i].index;
		pHBone->invGlobalTTM = pHmodelFormat->pBone[i].invGlobalTTM;
		pHBone->localTTM = Matrix();
		pHBone->name = pHmodelFormat->pBone[i].name;
		
		if (pHmodelFormat->pBone[i].bIsDepthZero)
		{
			pModel->rawData.pDepthZeroBoneNodes.push_back(pHBone.get());
		}

		pModel->rawData.pAllBoneNodes.push_back(std::move(pHBone));
	}

	for (int i = 0; i < pHmodelFormat->header.boneCount; i++)
	{
		if (pHmodelFormat->pBone[i].childCount != 0)
		{
			for (int j = 0; j < pHmodelFormat->pBone[i].childCount; j++)
			{
				pModel->rawData.pAllBoneNodes[i]->pChildBone.
					push_back(pModel->rawData.pAllBoneNodes[pHmodelFormat->pBone[i].pChildIndex[j]].get());
			}
		}
	}


	CreateRaytracingResource(pModel, resourceBatch);
	m_bModelDirty = true;

}

void HModelManager::CreateModelFromImportedFbx(HFbxImporter* pFbxImporter, ResourceUploadBatch& resourceBatch, HModel* pModel)
{
	auto pDevice = m_pDeviceResources->GetD3DDevice();

	HVertex* pHVertex = reinterpret_cast<HVertex*>(pFbxImporter->m_allVertices.data());


	pModel->rawData.vertexData.assign(pHVertex, pHVertex + pFbxImporter->m_allVertices.size());
	pModel->rawData.indexData.assign(pFbxImporter->m_allIndices.begin(), pFbxImporter->m_allIndices.end());

	pModel->rawData.AxisSystempModify = pFbxImporter->m_axisChange;

	pModel->rawData.boneCount = pFbxImporter->m_boneData.size();

	pModel->rawData.boundingBox = pFbxImporter->m_boundingBox;

	XMMATRIX axisModify = XMLoadFloat4x4(&pModel->rawData.AxisSystempModify);

	pModel->rawData.boundingBox.Transform(pModel->rawData.boundingBox, axisModify);

	for (int i = 0; i < pFbxImporter->m_meshData.size(); i++)
	{
		std::unique_ptr<HMesh> pHNode = std::make_unique<HMesh>();
		CreateAllMeshData(pFbxImporter->m_meshData[i], pHNode.get());
		pModel->rawData.pAllMeshNodes.push_back(std::move(pHNode));
	}

	for (int i = 0; i < pFbxImporter->m_meshData.size(); i++)
	{
		LinkMeshChild(pFbxImporter->m_meshData[i], pModel->rawData.pAllMeshNodes[i].get() , pModel);
	}

	for (int i = 0; i < pFbxImporter->m_boneData.size(); i++)
	{
		std::unique_ptr<HBone> pHBone = std::make_unique<HBone>();
		CreateAllBoneData(pFbxImporter->m_boneData[i], pHBone.get());
		pModel->rawData.pAllBoneNodes.push_back(std::move(pHBone));
	}

	for (int i = 0; i < pFbxImporter->m_boneData.size(); i++)
	{
		LinkBoneChild(pFbxImporter->m_boneData[i], pModel->rawData.pAllBoneNodes[i].get(), pModel);
	}

	PickZeroDepthMeshBone(pFbxImporter, pModel);
	CreateRaytracingResource(pModel, resourceBatch);

	m_bModelDirty = true;
}



void HModelManager::CreateAllMeshData(MeshData* pFbxMeshNode, HMesh* pHNode)
{
	pHNode->indexCount = pFbxMeshNode->indexCount;
	pHNode->vertexCount = pFbxMeshNode->vertexCount;
	pHNode->indexOffset = pFbxMeshNode->indexOffset;
	pHNode->vertexOffset = pFbxMeshNode->vertexOffset;
	pHNode->name = pFbxMeshNode->nodeName;

}

void HModelManager::LinkMeshChild(MeshData* pFbxMeshNode, HMesh* pHNode, HModel* pModel)
{
	if (pFbxMeshNode->childIndex.size() != 0)
	{
		for (int i = 0; i < pFbxMeshNode->childIndex.size(); i++)
		{
			pHNode->pChildNode.push_back(pModel->rawData.pAllMeshNodes[pFbxMeshNode->childIndex[i]].get());
		}
	}

}

//void HModelManager::CreatePerBoneDataRecursively(BoneData* pFbxBoneNode, HBone* pHBone, HBone* pParentBoneNode)
//{
//	pHBone->boneIndex = pFbxBoneNode->boneIndex;
//	pHBone->invGlobalTTM = pFbxBoneNode->invGlobalTTM;
//	pHBone->localTTM = pFbxBoneNode->localTTM;
//	pHBone->name = pFbxBoneNode->nodeName;
//	for (int i = 0; i < pFbxBoneNode->pChildNodes.size(); i++)
//	{
//		std::unique_ptr<HBone> pChildBone = std::make_unique<HBone>();
//		CreatePerBoneDataRecursively((BoneData*)pFbxBoneNode->pChildNodes[i], pChildBone.get(), pParentBoneNode);
//		pHBone->pChildBone.push_back(std::move(pChildBone));
//	}
//}

void HModelManager::CreateAllBoneData(BoneData* pFbxBoneNode, HBone* pHBone)
{
	pHBone->boneIndex = pFbxBoneNode->index;
	pHBone->invGlobalTTM = pFbxBoneNode->invGlobalTTM;
	pHBone->localTTM = pFbxBoneNode->localTTM;
	pHBone->name = pFbxBoneNode->nodeName;
}

void HModelManager::LinkBoneChild(BoneData* pFbxBoneNode, HBone* pHBone, HModel* pModel)
{
	for (int i = 0; i < pFbxBoneNode->childIndex.size(); i++)
	{
		pHBone->pChildBone.push_back(pModel->rawData.pAllBoneNodes[pFbxBoneNode->childIndex[i]].get());
	}
}

void HModelManager::PickZeroDepthMeshBone(HFbxImporter* pFbxImporter, HModel* pModel)
{
	for (int i = 0; i < pFbxImporter->m_meshData_parents.size(); i++)
	{
		pModel->rawData.pDepthZeroMeshNodes.push_back(
			pModel->rawData.pAllMeshNodes[pFbxImporter->m_meshData_parents[i]->index].get());
	}

	for (int i = 0; i < pFbxImporter->m_boneData_parents.size(); i++)
	{
		pModel->rawData.pDepthZeroBoneNodes.push_back(
			pModel->rawData.pAllBoneNodes[pFbxImporter->m_boneData_parents[i]->index].get());
	}

}

void HModelManager::CreateRaytracingResource(HModel* pModel, ResourceUploadBatch& resourceBatch)
{
	if (m_pDeviceResources->IsRaytracingSupported())
	{
		auto pDevice = m_pDeviceResources->GetD3DDevice();
		auto pRaytracingDevice = m_pDeviceResources->GetRaytracingDevice();

		CreateStaticBuffer(pDevice, resourceBatch, pModel->rawData.vertexData, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, pModel->vertexBuffer.GetAddressOf());
		CreateStaticBuffer(pDevice, resourceBatch, pModel->rawData.indexData, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, pModel->indexBuffer.GetAddressOf());

		//std::vector<XMFLOAT3X4A> transformMats;
		//transformMats.reserve(pFbxImporter->m_totalNodeCount);

		//for (int i = 0; i < pModel->pMeshNodes.size(); i++)
		//	CreateNodesTransformMatrixBufferRecursively(pModel->pMeshNodes[i].get(), pModel->AxisSystempModify, transformMats);

		//CreateStaticBuffer(pDevice, resourceBatch, transformMats,
		//	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, pModel->transformMatrixBuffer.GetAddressOf());

		//std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> rtDescs;
		//UINT transformBufferIndex = 0;

		//for (int i = 0; i < pModel->pMeshNodes.size(); i++)
		//	CreateRaytracingGeometryDescRecursively(pModel->pMeshNodes[i].get(), pModel, transformBufferIndex, rtDescs);

		D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;
		CreateRaytracingGeometryDesc(pModel, geometryDesc);

		//Bottom-level 가속화 구조체의 필요크기를 조회
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		//업데이트를 할 것이면 D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE를 옵션으로 준다.
		inputs.NumDescs = 1;//rtDescs.size();
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.pGeometryDescs = &geometryDesc;//rtDescs.data();

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild;
		pRaytracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);

		CD3DX12_HEAP_PROPERTIES defaultHeapProp(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC stratchResourceDesc =
			CD3DX12_RESOURCE_DESC::Buffer(prebuild.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		CD3DX12_RESOURCE_DESC resultResourceDesc =
			CD3DX12_RESOURCE_DESC::Buffer(prebuild.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		pRaytracingDevice->CreateCommittedResource(&defaultHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&stratchResourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&pModel->bottomLevelAccelerationStructureScratchBuffer));

		pRaytracingDevice->CreateCommittedResource(&defaultHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&resultResourceDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			IID_PPV_ARGS(&pModel->bottomLevelAccelerationStructureBuffer));

		//CommandList 인터페이스를 이용해서 가속화 구조체를 작성
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
		buildDesc.DestAccelerationStructureData = pModel->bottomLevelAccelerationStructureBuffer->GetGPUVirtualAddress();
		buildDesc.Inputs = inputs;
		buildDesc.ScratchAccelerationStructureData = pModel->bottomLevelAccelerationStructureScratchBuffer->GetGPUVirtualAddress();

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;
		DX::ThrowIfFailed(resourceBatch.GetCommandList()->QueryInterface(IID_PPV_ARGS(&commandList)));

		commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
	}
}



//void HModelManager::CreateNodesTransformMatrixBufferRecursively(HMesh* pNode, Matrix AxisChange, std::vector<XMFLOAT3X4A>& transformMats)
//{
//	XMFLOAT3X4A transformMatrix;
//
//	//DirectX::XMStoreFloat3x4A(&transformMatrix, pNode->staticTM * AxisChange);
//	transformMats.push_back(transformMatrix);
//
//	for (int i = 0; i < pNode->pChildNode.size(); i++)
//	{
//		CreateNodesTransformMatrixBufferRecursively(pNode->pChildNode[i], AxisChange, transformMats);
//	}
//}

//void HModelManager::CreateRaytracingGeometryDescRecursively(HMesh* pNode, HModel* pModel, UINT& transformBufferIndex, std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& rtDescs)
//{
//	{
//		D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC triangleDesc;
//
//		triangleDesc.VertexBuffer.StartAddress = pModel->vertexBuffer->GetGPUVirtualAddress() + sizeof(HVertex) * pNode->vertexOffset;
//		triangleDesc.VertexBuffer.StrideInBytes = sizeof(HVertex);
//		triangleDesc.VertexCount = pNode->vertexCount;
//		triangleDesc.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
//
//		triangleDesc.IndexBuffer = pModel->indexBuffer->GetGPUVirtualAddress() + sizeof(std::uint32_t) * pNode->indexOffset;
//		triangleDesc.IndexFormat = DXGI_FORMAT_R32_UINT;
//		triangleDesc.IndexCount = pNode->indexCount;
//
//		//triangleDesc.Transform3x4 = pModel->transformMatrixBuffer->GetGPUVirtualAddress() + sizeof(XMFLOAT3X4A) * transformBufferIndex;
//		triangleDesc.Transform3x4 = 0;
//
//
//		transformBufferIndex++;//transformMatrixBuffer는 노드의 깊이 우선 순서에 따라 만들어졌으므로 참조할때도 같은 방식으로 참조한다.
//
//		D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;
//
//		geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
//		geometryDesc.Triangles = triangleDesc;
//		geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
//		//스키닝에서는 FLAG_ALLOW_UPDATE이 필요할 수 있다.
//		//성능 향성을 위해 가능하면 D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE로 한다.
//		//Hazumu 예시에서는 D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE만 옵션으로 주었다.
//
//		rtDescs.push_back(geometryDesc);
//	}
//
//	for (int i = 0; i < pNode->pChildNode.size(); i++)
//	{
//		CreateRaytracingGeometryDescRecursively(pNode->pChildNode[i].get(), pModel, transformBufferIndex, rtDescs);
//	}
//}

void HModelManager::CreateRaytracingGeometryDesc(HModel* pModel, D3D12_RAYTRACING_GEOMETRY_DESC& rtDescs)
{
	D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC triangleDesc;

	triangleDesc.VertexBuffer.StartAddress = pModel->vertexBuffer->GetGPUVirtualAddress();
	triangleDesc.VertexBuffer.StrideInBytes = sizeof(HVertex);
	triangleDesc.VertexCount = pModel->rawData.vertexData.size();
	triangleDesc.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

	triangleDesc.IndexBuffer = pModel->indexBuffer->GetGPUVirtualAddress();
	triangleDesc.IndexFormat = DXGI_FORMAT_R32_UINT;
	triangleDesc.IndexCount = pModel->rawData.indexData.size();

	//triangleDesc.Transform3x4 = pModel->transformMatrixBuffer->GetGPUVirtualAddress() + sizeof(XMFLOAT3X4A) * transformBufferIndex;
	triangleDesc.Transform3x4 = 0;

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;

	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles = triangleDesc;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	//스키닝에서는 FLAG_ALLOW_UPDATE이 필요할 수 있다.
	//성능 향성을 위해 가능하면 D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE로 한다.
	//Hazumu 예시에서는 D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE만 옵션으로 주었다.

	rtDescs = geometryDesc;
}

void HModelManager::Update()
{
	MergeVertexIndexData();
	CheckInstanceAlive();
	CheckFrustumCulling();
	//CheckInstanceCount();
}

D3D12_VERTEX_BUFFER_VIEW HModelManager::GetVertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = m_allVerticesResource->GetGPUVirtualAddress();
	vbv.StrideInBytes = sizeof(HVertex);
	vbv.SizeInBytes = sizeof(HVertex) * m_allVerticesCount;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW HModelManager::GetIndexBufferView() const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = m_allIndicesResource->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R32_UINT;
	ibv.SizeInBytes = sizeof(std::uint32_t) * m_allIndicesCount;

	return ibv;
}

void HModelManager::CheckInstanceAlive()
{
	std::vector<void*> toBeDeletedList;

	for (auto& e : m_instances)
	{
		HInstance* pInstance = e.second.lock().get();

		if (pInstance == nullptr)
		{
			toBeDeletedList.push_back(e.first);
		}
		else
		{
			//if()
			//if (pInstance.get()->pMat.expired()|| pInstance.get()->pMatSecond.expired())
			//	toBeDeletedList_temp.push_back(e.first);
		}
	}

	for (auto& e : toBeDeletedList)
	{
		m_instances.erase(e);
		m_bInstanceDirty = true;
	}
}

void HModelManager::MergeVertexIndexData()
{
	auto pDevice = m_pDeviceResources->GetD3DDevice();

	if (m_bModelDirty == true)
	{
		m_allVertices.clear();
		m_allIndices.clear();

		UINT vertexOffset = 0;
		UINT indexOffset = 0;

		for (auto& model : m_models)
		{
			HModel* pModel = model.second.get();

			pModel->vertexOffsetInEntireBuffer = vertexOffset;
			pModel->indexOffsetInEntireBuffer = indexOffset;

			m_allVertices.insert(m_allVertices.end(),
				pModel->rawData.vertexData.begin(), pModel->rawData.vertexData.end());

			m_allIndices.insert(m_allIndices.end(),
				pModel->rawData.indexData.begin(), pModel->rawData.indexData.end());

			for (int i = 0; i < pModel->rawData.pDepthZeroMeshNodes.size(); i++)
				CalNodeVertexIndexOffsetRecursively(pModel->rawData.pDepthZeroMeshNodes[i], vertexOffset, indexOffset);

			vertexOffset = m_allVertices.size();
			indexOffset = m_allIndices.size();
		}

		ResourceUploadBatch batch(pDevice);
		batch.Begin();

		CreateStaticBuffer(pDevice, batch, m_allVertices,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			| D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			m_allVerticesResource.ReleaseAndGetAddressOf());

		CreateStaticBuffer(pDevice, batch, m_allIndices,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			| D3D12_RESOURCE_STATE_INDEX_BUFFER,
			m_allIndicesResource.ReleaseAndGetAddressOf());

		auto uploadResourcesFinished = batch.End(m_pDeviceResources->GetCommandQueue());
		uploadResourcesFinished.wait();

		m_allVerticesCount = m_allVertices.size();
		m_allIndicesCount = m_allIndices.size();
		m_bModelDirty = false;
	}
}

void HModelManager::CheckInstanceCount()
{
	for (auto& model : m_models)
	{
		UINT instanceCount = 0;

		for (auto& instancesPerFlag : model.second->instances)
		{
			instanceCount += instancesPerFlag.second.size();
		}

		model.second->visibleInstanceCount = instanceCount;
	}
}

void HModelManager::CheckFrustumCulling()
{
	XMMATRIX view = m_pCamera->GetView();
	XMVECTOR det;
	Matrix invView = XMMatrixInverse(&det, view);

	m_visibleInstance = 0;
	
	if (m_pCamera->GetCamMode() == CameraMode::ePerspective)
	{
		const BoundingFrustum& boundingFrustum = m_pCamera->GetPerspectiveBoudingFrustum();

		for (auto& e : m_models)
		{
			HModel* pModel = e.second.get();
			pModel->visibleInstanceCount_temp = 0;
			
			for (unsigned char flag = 0; flag <= ALLOPTION_MM; flag++)
			{
				pModel->visibleInstances[flag].clear();

				for (auto& instance : pModel->instances[flag])
				{
					HInstance* pInstance = instance.second.get();

					Matrix world = pInstance->worldTM;
					Matrix invWorld = world.Invert();

					Matrix viewToLocal;
					viewToLocal *= invView;
					viewToLocal *= invWorld;

					BoundingFrustum localFrustum;
					boundingFrustum.Transform(localFrustum, viewToLocal);

					if (localFrustum.Contains(pModel->rawData.boundingBox) != DirectX::DISJOINT)
					{
						pInstance->bIsInCameraRange = true;
						pModel->visibleInstances[flag].push_back(pInstance);
						pModel->visibleInstanceCount_temp++;
						m_visibleInstance++;
					}
					else
					{
						pInstance->bIsInCameraRange = false;
					}
				}
			}
		}
	}
	else
	{
		const BoundingBox& boundingBox = m_pCamera->GetOrthographicBoudingBox();

		for (auto& e : m_models)
		{
			HModel* pModel = e.second.get();
			pModel->visibleInstanceCount_temp = 0;


			for (unsigned char flag = 0; flag <= ALLOPTION_MM; flag++)
			{
				pModel->visibleInstances[flag].clear();

				for (auto& instance : pModel->instances[flag])
				{
					HInstance* pInstance = instance.second.get();

					Matrix world = pInstance->worldTM;
					Matrix invWorld = world.Invert();

					Matrix viewToLocal;
					viewToLocal *= invView;
					viewToLocal *= invWorld;

					BoundingBox localBox;
					boundingBox.Transform(localBox, viewToLocal);

					if (localBox.Contains(pInstance->pModel.lock().get()->rawData.boundingBox) != DirectX::DISJOINT)
					{
						pInstance->bIsInCameraRange = true;
						pModel->visibleInstances[flag].push_back(pInstance);
						pModel->visibleInstanceCount_temp++;
						m_visibleInstance++;
					}
					else
					{
						pInstance->bIsInCameraRange = false;
					}
				}
			}
		}
	}

}

void HModelManager::CalNodeVertexIndexOffsetRecursively(HMesh* pNode, UINT ModelVertexOffset, UINT ModelIndexOffset)
{
	pNode->vertexOffsetInEntireBuffer = pNode->vertexOffset + ModelVertexOffset;
	pNode->indexOffsetInEntireBuffer = pNode->indexOffset + ModelIndexOffset;

	for (int i = 0; i < pNode->pChildNode.size(); i++)
	{
		CalNodeVertexIndexOffsetRecursively(pNode->pChildNode[i], ModelVertexOffset, ModelIndexOffset);
	}
}

void HModelManager::UpdateResource_raterize()
{
	for (auto& e : m_models)
	{
		HModel* model = e.second.get();
		UINT boneCnt = model->rawData.boneCount;

		UpdatePerModelResouce_rasterize(model);

		UINT instanceOffset = 0;
		for (unsigned char flag = 0; flag <= ALLOPTION_MM; flag++)
		{
			UpdatePerPSOResource_rasterize(model, flag, instanceOffset);
			UpdatePerInstanceResouce_rasterize(model, flag);
		}
	}
}

void HModelManager::UpdatePerModelResouce_rasterize(HModel* pHModel)
{
	//perModelData 업데이트, 계속 해줄 필요는 없고 초기화때만 해주면 될 것 같다.※
	HConstantBuffer_PerModel_Rasterize CB_perModel;
	CB_perModel.boneCount = pHModel->rawData.boneCount;
	CB_perModel.axisChange = pHModel->rawData.AxisSystempModify.Transpose();
	pHModel->perModelCB.Reset(m_pGraphicMemory->AllocateConstant(CB_perModel));
}

void HModelManager::UpdatePerInstanceResouce_rasterize(HModel* pHModel, unsigned char shaderFlag)
{
	UpdatePerInstanceStructuredBuffer_rasterize(pHModel->visibleInstances[shaderFlag], &pHModel->structuredBuffer_instances[shaderFlag]);
}

void HModelManager::UpdatePerPSOResource_rasterize(HModel* pHModel, unsigned char shaderFlag, UINT& instanceOffset)
{
	HStructuredBuffer_PerPSO_Rasterize perPSOData;
	perPSOData.instanceNumOffset = instanceOffset;

	UINT bufferSize = sizeof(HStructuredBuffer_PerPSO_Rasterize);

	SharedGraphicsResource graphicResourceCB =
		m_pGraphicMemory->Allocate(bufferSize);

	memcpy(graphicResourceCB.Memory(),
		&perPSOData,
		bufferSize);

	pHModel->structuredBuffer_perPso[shaderFlag].Reset(graphicResourceCB);

	instanceOffset += pHModel->visibleInstances[shaderFlag].size();
}



void HModelManager::UpdatePerInstanceStructuredBuffer_rasterize(/*std::unordered_map<void*, std::shared_ptr<HInstance>> const& instances*/
	std::vector<HInstance*>& instances, SharedGraphicsResource* pStructuredBuffer)
{
	if (instances.size() == 0)
		return;

	std::vector<HStructuredBuffer_PerInstance_Rasterize> structuredBuffer_perInstance;
	structuredBuffer_perInstance.reserve(instances.size());

	for (auto& e : instances)
	{
		HInstance* pHinstance = e;

		HStructuredBuffer_PerInstance_Rasterize currInstance;
		currInstance.worldTM = pHinstance->worldTM.Transpose();

		UINT* pMatIndex = pHinstance->pMat.lock().get();
		if (pMatIndex == nullptr)
			currInstance.materialIndex0 = -1;
		else
			currInstance.materialIndex0 = *pMatIndex;

		pMatIndex = pHinstance->pMatSecond.lock().get();
		if (pMatIndex == nullptr)
			currInstance.materialIndex1 = -1;
		else
			currInstance.materialIndex1 = *pMatIndex;

		currInstance.materialLerpValue = pHinstance->materialLerpValue;

		structuredBuffer_perInstance.push_back(currInstance);
	}

	UINT bufferSize = structuredBuffer_perInstance.size() *
		sizeof(HStructuredBuffer_PerInstance_Rasterize);

	SharedGraphicsResource graphicResourceCB =
		m_pGraphicMemory->Allocate(bufferSize);

	memcpy(graphicResourceCB.Memory(),
		structuredBuffer_perInstance.data(),
		bufferSize);

	pStructuredBuffer->Reset(graphicResourceCB);
}

//void HModelManager::UpdateperInstanceBoneAnimationResouce_rasterize(HModel* pHModel, unsigned char shaderFlag)
//{
//	UINT boneCount = pHModel->rawData.boneCount;
//	std::vector<Matrix> boneData;
//
//	for (auto& instance : pHModel->instances[shaderFlag])
//	{
//		HInstance* pHinstance = instance.second.get();
//		UpdateInstanceBoneAnimResource_common(boneData, pHinstance, pHModel);
//	}
//
//	if (boneData.size() != 0)
//	{
//		UINT bufferSize = boneData.size() * sizeof(Matrix);
//		SharedGraphicsResource graphicResourceCB =
//			m_pGraphicMemory->Allocate(bufferSize);
//		memcpy(graphicResourceCB.Memory(),
//			boneData.data(),
//			bufferSize);
//
//		pHModel->structuredBuffer_bones[shaderFlag].Reset(graphicResourceCB);
//	}
//}

//bone TM 구성 
//예시 : 모델1(본 숫자 3, 인스턴스2) + 모델2(본x, 인스턴스2) + 모델3(본 숫자2, 인스턴스1)
//TM 구성 : 모델1(인스턴스1(TM1,TM2,TM3), 인스턴스2(TM1,TM2,TM3)) + 모델2(데이터없음) + 모델3(인스턴스1(TM1,TM2),인스턴스2(TM1,TM2))

void HModelManager::UpdateAllInstancedBoneAnimResource_common()
{
	for (auto& e : m_models)
	{
		std::vector<Matrix> boneData;

		HModel* pHModel = e.second.get();

		if (pHModel->rawData.boneCount == 0)
			continue;

		for (unsigned char flag = 0; flag <= ALLOPTION_MM; flag++)
		{
			for (auto& instance : pHModel->visibleInstances[flag])
			{
				//HInstance* pHinstance = instance.second.get();

				UpdateInstanceBoneAnimResource_common(boneData, instance, pHModel);
			}
		}

		if (boneData.size() != 0)
		{
			UINT bufferSize = boneData.size() * sizeof(Matrix);
			SharedGraphicsResource graphicResourceCB =
				m_pGraphicMemory->Allocate(bufferSize);
			memcpy(graphicResourceCB.Memory(),
				boneData.data(),
				bufferSize);

			pHModel->structuredBuffer_computeShader_boneTMs.Reset(graphicResourceCB);
		}
	}
}

void HModelManager::UpdateInstanceBoneAnimResource_common(std::vector<Matrix>& allBoneTM, HInstance* pHinstance, HModel* pHModel)
{
	UINT boneCount = pHModel->rawData.boneCount;

	if (pHinstance->animationName.empty() && pHinstance->animationNameSecond.empty())
	{
		std::vector<Matrix> indentityMats(boneCount);
		allBoneTM.insert(allBoneTM.end(), indentityMats.begin(), indentityMats.end());
	}
	else
	{
		HBoneAnims* pAnim = nullptr;
		HBoneAnims* pAnim_second = nullptr;
		float firstAnimTime = 0;
		float secondAnimTime = 0;

		if (!pHinstance->animationName.empty())
		{
			pAnim = pHModel->m_pAnim->rawData.animBundle[pHinstance->animationName].get();
			//pAnim = pHModel->rawData.animations[pHinstance->animationName].get();
			if (pAnim == nullptr)
				HerrorMessage(L"Instance's first animation name does not match with Model's data.")
			else
			{
				float totalTime = pAnim->totalTime;
				firstAnimTime = fmodf(pHinstance->animationTime, totalTime);
			}
		}

		if (!pHinstance->animationNameSecond.empty())
		{
			pAnim_second = pHModel->m_pAnim->rawData.animBundle[pHinstance->animationNameSecond].get();
			//pAnim_second = pHModel->rawData.animations[pHinstance->animationNameSecond].get();
			if (pAnim_second == nullptr)
				HerrorMessage(L"Instance's second animation name does not match with Model's data.")
			else
			{
				float totalTime = pAnim_second->totalTime;
				secondAnimTime = fmodf(pHinstance->animationTimeSecond, totalTime);
			}
		}

		//
		Matrix identityTM;
		for (int j = 0; j < pHModel->rawData.pDepthZeroBoneNodes.size(); j++)
		{
			//UpdateBoneAnimRecusively_local_common(boneData, currTime, pHModel->rawData.pBoneNodes[j].get(), pAnim, identityTM);
			UpdateBoneAnimRecusively_local_common(allBoneTM, pHinstance->boneTMCurrFrame, pHModel->rawData.pDepthZeroBoneNodes[j],
				pAnim, pAnim_second, firstAnimTime, secondAnimTime, pHinstance->animationLerpValue, identityTM);

		}
	}

}
//
//void HModelManager::UpdateBoneAnimRecusively_local_common(std::vector<Matrix>& boneData,
//	float animTime, HBone* pBone, HAnim* pAnim, Matrix parentTM)
//{
//	UINT boneIndex = pBone->boneIndex;
//	Matrix invGlobalTTM = pBone->invGlobalTTM;
//	Matrix localTTM = pBone->localTTM;
//
//	const HBoneAnim& hBoneAnim = pAnim->boneAnim[boneIndex];
//
//	Matrix currBoneTM;
//
//	if (hBoneAnim.animKeys.size() == 0)
//	{
//		currBoneTM = localTTM;
//	}
//	else
//	{
//		HAnimKey firstKey = hBoneAnim.animKeys.front();
//		HAnimKey lastKey = hBoneAnim.animKeys.back();
//
//		if (animTime <= firstKey.time)
//		{
//			XMVECTOR vQuatRot = XMLoadFloat4(&firstKey.quatRot);
//			XMVECTOR vTrans = XMLoadFloat3(&firstKey.trans);
//			XMVECTOR vScale = XMLoadFloat3(&firstKey.scale);
//
//			XMMATRIX mRot = XMMatrixRotationQuaternion(vQuatRot);
//			XMMATRIX mTras = XMMatrixTranslationFromVector(vTrans);
//			XMMATRIX mScale = XMMatrixScalingFromVector(vScale);
//
//			XMMATRIX mTransform = mScale * mRot * mTras;
//
//			XMStoreFloat4x4(&currBoneTM, mTransform);
//		}
//		else if (animTime >= lastKey.time)
//		{
//			XMVECTOR vQuatRot = XMLoadFloat4(&lastKey.quatRot);
//			XMVECTOR vTrans = XMLoadFloat3(&lastKey.trans);
//			XMVECTOR vScale = XMLoadFloat3(&lastKey.scale);
//
//			XMMATRIX mRot = XMMatrixRotationQuaternion(vQuatRot);
//			XMMATRIX mTras = XMMatrixTranslationFromVector(vTrans);
//			XMMATRIX mScale = XMMatrixScalingFromVector(vScale);
//
//			XMMATRIX mTransform = mScale * mRot * mTras;
//
//			XMStoreFloat4x4(&currBoneTM, mTransform);
//		}
//		else
//		{
//			HAnimKey currKey;
//			HAnimKey nextKey;
//			for (int i = 0; i + 1 < hBoneAnim.animKeys.size(); i++)
//			{
//				if (animTime > hBoneAnim.animKeys[i].time && animTime < hBoneAnim.animKeys[i + 1].time)
//				{
//					currKey = hBoneAnim.animKeys[i];
//					nextKey = hBoneAnim.animKeys[i + 1];
//					break;
//				}
//			}
//
//			float totalTime = nextKey.time - currKey.time;
//			float lerpValue = (animTime - currKey.time) / totalTime;
//
//			XMVECTOR vQuatRot0 = XMLoadFloat4(&currKey.quatRot);
//			XMVECTOR vTrans0 = XMLoadFloat3(&currKey.trans);
//			XMVECTOR vScale0 = XMLoadFloat3(&currKey.scale);
//
//			XMVECTOR vQuatRot1 = XMLoadFloat4(&nextKey.quatRot);
//			XMVECTOR vTrans1 = XMLoadFloat3(&nextKey.trans);
//			XMVECTOR vScale1 = XMLoadFloat3(&nextKey.scale);
//
//			XMVECTOR vQuatRot = XMQuaternionSlerp(vQuatRot0, vQuatRot1, lerpValue);
//			XMVECTOR vTrans = XMVectorLerp(vTrans0, vTrans1, lerpValue);
//			XMVECTOR vScale = XMVectorLerp(vScale0, vScale1, lerpValue);
//
//			XMMATRIX mRot = XMMatrixRotationQuaternion(vQuatRot);
//			XMMATRIX mTras = XMMatrixTranslationFromVector(vTrans);
//			XMMATRIX mScale = XMMatrixScalingFromVector(vScale);
//
//			XMMATRIX mTransform = mScale * mRot * mTras;
//
//			XMStoreFloat4x4(&currBoneTM, mTransform);
//		}
//	}
//	currBoneTM = currBoneTM * parentTM;
//
//	Matrix TMforSave = (invGlobalTTM * currBoneTM).Transpose();
//	boneData.push_back(TMforSave);
//
//	for (int i = 0; i < pBone->pChildBone.size(); i++)
//	{
//		UpdateBoneAnimRecusively_local_common(boneData, animTime, pBone->pChildBone[i].get(), pAnim, currBoneTM);
//	}
//
//
//}

void HModelManager::UpdateBoneAnimRecusively_local_common(std::vector<Matrix>& allBoneData, std::unordered_map<std::string, Matrix>& instanceBoneTM,
	HBone* pBone, HBoneAnims* pAnim, HBoneAnims* pSecondAnim, float animTime, float animTimeSecond, float lerpValue, Matrix parentTM)
{
	UINT boneIndex = pBone->boneIndex;
	Matrix invGlobalTTM = pBone->invGlobalTTM;
	Matrix localTTM = pBone->localTTM;

	Vector3 vScale = XMVectorSet(1, 1, 1, 1);
	Quaternion vQuatRot = XMVectorSet(0, 0, 0, 1);
	Vector3 vTrans = XMVectorSet(0, 0, 0, 1);

	Vector3 vScale_second = XMVectorSet(1, 1, 1, 1);
	Quaternion vQuatRot_second = XMVectorSet(0, 0, 0, 1);
	Vector3 vTrans_second = XMVectorSet(0, 0, 0, 1);

	if (pAnim != nullptr)
	{
		const HBoneAnim& hBoneAnim = pAnim->boneAnim[boneIndex];
		CalculateBoneAnim(&localTTM, &hBoneAnim, animTime, vScale, vQuatRot, vTrans);
	}

	if (pSecondAnim != nullptr)
	{
		const HBoneAnim& hBoneAnim_second = pSecondAnim->boneAnim[boneIndex];
		CalculateBoneAnim(&localTTM, &hBoneAnim_second, animTimeSecond, vScale_second, vQuatRot_second, vTrans_second);
	}

	Vector3 scale_final;
	Quaternion quat_final;
	Vector3 trans_final;

	if (pAnim != nullptr && pSecondAnim != nullptr)
	{
		scale_final = Vector3::Lerp(vScale, vScale_second, lerpValue);
		quat_final = Quaternion::Lerp(vQuatRot, vQuatRot_second, lerpValue);
		trans_final = Vector3::Lerp(vTrans, vTrans_second, lerpValue);
	}
	else if (pAnim != nullptr)
	{
		scale_final = vScale;
		quat_final = vQuatRot;
		trans_final = vTrans;
	}
	else if (pSecondAnim != nullptr)
	{
		scale_final = vScale_second;
		quat_final = vQuatRot_second;
		trans_final = vTrans_second;
	}

	Matrix currBoneTM;

	currBoneTM *= Matrix::CreateScale(scale_final);
	currBoneTM *= Matrix::CreateFromQuaternion(quat_final);
	currBoneTM *= Matrix::CreateTranslation(trans_final);

	currBoneTM *= parentTM;

	instanceBoneTM[pBone->name] = invGlobalTTM * currBoneTM;
	Matrix TMforSave = (invGlobalTTM * currBoneTM).Transpose();
	allBoneData.push_back(TMforSave);

	for (int i = 0; i < pBone->pChildBone.size(); i++)
	{
		UpdateBoneAnimRecusively_local_common(allBoneData, instanceBoneTM, pBone->pChildBone[i], pAnim, pSecondAnim, animTime, animTimeSecond, lerpValue, currBoneTM);
	}
}

void HModelManager::CalculateBoneAnim(Matrix* boneLocalTM, HBoneAnim const* pBoneAnim, float time, Vector3& outScale, Quaternion& outQuat, Vector3& outTrans)
{

	if (pBoneAnim->animKeys.size() == 0)
	{
		boneLocalTM->Decompose(outScale, outQuat, outTrans);
	}
	else
	{
		HAnimKey firstKey = pBoneAnim->animKeys.front();
		HAnimKey lastKey = pBoneAnim->animKeys.back();

		if (time <= firstKey.time)
		{
			outScale = firstKey.scale;
			outQuat = firstKey.quatRot;
			outTrans = firstKey.trans;
		}
		else if (time >= lastKey.time)
		{
			outScale = lastKey.scale;
			outQuat = lastKey.quatRot;
			outTrans = lastKey.trans;
		}
		else
		{
			HAnimKey currKey;
			HAnimKey nextKey;
			for (int i = 0; i + 1 < pBoneAnim->animKeys.size(); i++)
			{
				if (time > pBoneAnim->animKeys[i].time && time < pBoneAnim->animKeys[i + 1].time)
				{
					currKey = pBoneAnim->animKeys[i];
					nextKey = pBoneAnim->animKeys[i + 1];
					break;
				}
			}

			float timeBetweenTwoKey = nextKey.time - currKey.time;
			float lerpValue = (time - currKey.time) / timeBetweenTwoKey;

			XMVECTOR vQuatRot0 = XMLoadFloat4(&currKey.quatRot);
			XMVECTOR vTrans0 = XMLoadFloat3(&currKey.trans);
			XMVECTOR vScale0 = XMLoadFloat3(&currKey.scale);

			XMVECTOR vQuatRot1 = XMLoadFloat4(&nextKey.quatRot);
			XMVECTOR vTrans1 = XMLoadFloat3(&nextKey.trans);
			XMVECTOR vScale1 = XMLoadFloat3(&nextKey.scale);

			outQuat = XMQuaternionSlerp(vQuatRot0, vQuatRot1, lerpValue);
			outTrans = XMVectorLerp(vTrans0, vTrans1, lerpValue);
			outScale = XMVectorLerp(vScale0, vScale1, lerpValue);
		}

	}
}

//void HModelManager::UpdateBoneAnim_global(std::vector<Matrix>& boneData, float animTime, HBone* pBone, HBoneAnims* pAnim)
//{
//	UINT boneIndex = pBone->boneIndex;
//	Matrix invGlobalTTM = pBone->invGlobalTTM;
//	Matrix localTTM = pBone->localTTM;
//
//	const HBoneAnim& hBoneAnim = pAnim->boneAnim[boneIndex];
//
//	Matrix currBoneTM;
//
//	if (hBoneAnim.animKeys.size() == 0)
//	{
//		currBoneTM = localTTM;
//	}
//	else
//	{
//		HAnimKey firstKey = hBoneAnim.animKeys.front();
//		HAnimKey lastKey = hBoneAnim.animKeys.back();
//
//		if (animTime <= firstKey.time)
//		{
//			currBoneTM = firstKey.globalTM;
//
//		}
//		else if (animTime >= lastKey.time)
//		{
//			currBoneTM = lastKey.globalTM;
//		}
//		else
//		{
//			HAnimKey currKey;
//			HAnimKey nextKey;
//			for (int i = 0; i + 1 < hBoneAnim.animKeys.size(); i++)
//			{
//				if (animTime > hBoneAnim.animKeys[i].time && animTime < hBoneAnim.animKeys[i + 1].time)
//				{
//					currKey = hBoneAnim.animKeys[i];
//					nextKey = hBoneAnim.animKeys[i + 1];
//					break;
//				}
//			}
//
//			float totalTime = nextKey.time - currKey.time;
//			float lerpValue = (animTime - currKey.time) / totalTime;
//
//			XMMATRIX matrixCurr = XMLoadFloat4x4(&currKey.globalTM);
//			XMMATRIX matrixNext = XMLoadFloat4x4(&nextKey.globalTM);
//
//			XMVECTOR vQuatRotCurr;
//			XMVECTOR vTransCurr;
//			XMVECTOR vScaleCurr;
//			XMMatrixDecompose(&vScaleCurr, &vQuatRotCurr, &vTransCurr, matrixCurr);
//
//			XMVECTOR vQuatRotNext;
//			XMVECTOR vTransNext;
//			XMVECTOR vScaleNext;
//			XMMatrixDecompose(&vScaleNext, &vQuatRotNext, &vTransNext, matrixNext);
//
//			XMVECTOR vQuatRot = XMQuaternionSlerp(vQuatRotCurr, vQuatRotNext, lerpValue);
//			XMVECTOR vTrans = XMVectorLerp(vTransCurr, vTransNext, lerpValue);
//			XMVECTOR vScale = XMVectorLerp(vScaleCurr, vScaleNext, lerpValue);
//
//			XMVECTOR zero = XMVectorSet(0, 0, 0, 1);
//
//			XMMATRIX trasformMatrix = XMMatrixAffineTransformation(vScale, zero, vQuatRot, vTrans);
//
//			XMStoreFloat4x4(&currBoneTM, trasformMatrix);
//		}
//	}
//
//	Matrix TMforSave = (invGlobalTTM * currBoneTM).Transpose();
//	boneData.push_back(TMforSave);
//
//}


void HModelManager::UpdatePerNodeResouceRecursively_rasterize(HMesh* pNode, UINT modelBoneCount)
{
	//노드별 상수 버퍼 갱신 초기화할때 한번만 해주면 될 것 같다 ※
	HConstantBuffer_PerNode_Rasterize CBperNode;

	//if(modelBoneCount == 0)
	//	CBperNode.nodeTM = pNode->staticTM.Transpose();
	//else
	//	CBperNode.nodeTM = pNode->globalBindTM.Transpose();

	//pNode->perNodeCB.Reset(m_pGraphicMemory->AllocateConstant(CBperNode));

	for (int i = 0; i < pNode->pChildNode.size(); i++)
	{
		UpdatePerNodeResouceRecursively_rasterize(pNode->pChildNode[i], modelBoneCount);
	}
}




//void HModelManager::UpdateRaytracingStructuredBuffer()
//{
//	if (m_pDeviceResources->IsRaytracingSupported())
//	{
//		UpdatePerInstanceData();
//		UpdatePerModelData();
//		UpdatePerNodeData();
//	}
//}

void HModelManager::UpdateResouce_raytracing()
{
	UpdateAllInstancedVertices_raytracing();
	CreatePerDynamicModelDataForComputeShader();
	UpdatePerInstanceResource_raytracing();
	UpdatePerModelResource_raytracing();
}

void HModelManager::UpdatePerInstanceResource_raytracing()
{
	std::vector<HSturecturedBuffer_Raytracing_PerInstance> perInstanceResource_raytracing;
	perInstanceResource_raytracing.reserve(m_instances.size());

	UINT modelIndex = 0;

	for (auto& model : m_models)
	{
		HModel* pModel = model.second.get();

		HSturecturedBuffer_Raytracing_PerInstance currentInstanceResouce;

		currentInstanceResouce.modelIndex = modelIndex;
		modelIndex++;
		UINT instanceIndexInModel = 0;

		for (unsigned char flag = 0; flag <= ALLOPTION_MM; flag++)
		{
			for (auto& instance : pModel->visibleInstances[flag])
			{
				HInstance* pHInstance = instance;


				UINT* pMatIndex = pHInstance->pMat.lock().get();
				if (pMatIndex == nullptr)
					currentInstanceResouce.matIndex0 = -1;
				else
					currentInstanceResouce.matIndex0 = *pMatIndex;

				pMatIndex = pHInstance->pMatSecond.lock().get();
				if (pMatIndex == nullptr)
					currentInstanceResouce.matIndex1 = -1;
				else
					currentInstanceResouce.matIndex1 = *pMatIndex;

				currentInstanceResouce.lerpValue = pHInstance->materialLerpValue;
				currentInstanceResouce.instanceIndexInModel = instanceIndexInModel;
				instanceIndexInModel++;

				perInstanceResource_raytracing.push_back(currentInstanceResouce);
			}
		}
	}

	if (perInstanceResource_raytracing.size() != 0)
	{
		UINT bufferSize = sizeof(HSturecturedBuffer_Raytracing_PerInstance) * perInstanceResource_raytracing.size();

		SharedGraphicsResource graphicResource =
			m_pGraphicMemory->Allocate(bufferSize);

		memcpy(graphicResource.Memory(),
			perInstanceResource_raytracing.data(),
			bufferSize);

		m_raytracingStructuredBuffer_perInstance.Reset(graphicResource);
	}

}

void HModelManager::UpdateAllInstancedVertices_raytracing()
{
	static unsigned int lastFrameVertexCount = 0;
	//매번하지말고 인스턴스 변할 때마다 바뀌도록 코드 추가 해야함.※
	unsigned int totalVertexCount = GetTotalDynamicVertexCount();

	if (lastFrameVertexCount != totalVertexCount && totalVertexCount != 0)
	{
		CreateVertexBuffer_raytracing(totalVertexCount);
		lastFrameVertexCount = totalVertexCount;
	}
}


//예시 : 모델1(본O) + 모델2(본X) + 모델3(본O)
//버텍스 구성 : 모델1(인스턴스1, 인스턴스2, 인스턴스3) + 모델2(원본 버텍스) + 모델3(인스턴스1, 인스턴스2)

unsigned int HModelManager::GetTotalDynamicVertexCount()
{
	unsigned int totalDynamicVertex = 0;

	for (auto& model : m_models)
	{
		HModel* pModel = model.second.get();

		if (pModel->rawData.boneCount == 0)//no bone-> static model
		{
			continue;
		}

		for (unsigned char flag = 0; flag <= ALLOPTION_MM; flag++)
		{
			totalDynamicVertex += pModel->visibleInstances[flag].size() * pModel->rawData.vertexData.size();
		}
	}

	return totalDynamicVertex;
}

void HModelManager::CreateVertexBuffer_raytracing(unsigned int vertexCount)
{
	auto pDevice = m_pDeviceResources->GetD3DDevice();
	int currentFrame = m_pDeviceResources->GetCurrentFrameIndex();

	HBufferManager* pBufferManager = HBufferManager::GetInstance();

	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_RESOURCE_DESC DXRDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexCount * sizeof(HVertex_raytracing));
	DXRDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	pBufferManager->CreateDefaultHeapBuffer_device0(pDevice, DXRDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, m_pAllInstacedVertices);

	m_pAllInstacedVertices->SetName(L"VertexData_raytracing");

	//DX::ThrowIfFailed(pDevice->CreateCommittedResource(
	//	&heapProperties,
	//	D3D12_HEAP_FLAG_NONE,
	//	&DXRDesc,
	//	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,//나중에 UAV로 바꿔줄거임
	//	nullptr,
	//	IID_PPV_ARGS(m_pAllInstacedVertices[currentFrame].ReleaseAndGetAddressOf())
	//));

	//m_pAllInstacedVertices[currentFrame]->SetName(L"VertexData_raytracing");
}

void HModelManager::CreatePerDynamicModelDataForComputeShader()
{
	int vertexRecordOffset = 0;

	for (auto& model : m_models)
	{
		HModel* pModel = model.second.get();

		if (pModel->rawData.boneCount == 0)
			continue;

		HStructuredBuffer_Raytracing_ComputeShader_PerModel perModel;

		perModel.boneCount = pModel->rawData.boneCount;
		perModel.vertexCount = pModel->rawData.vertexData.size();

		int totalInstanceCount = 0;
		for (unsigned char flag = 0; flag <= ALLOPTION_MM; flag++)
		{
			totalInstanceCount += pModel->visibleInstances[flag].size();
		}

		perModel.instanceCount = totalInstanceCount;
		perModel.vertexRecordOffset = vertexRecordOffset;

		vertexRecordOffset += totalInstanceCount * perModel.vertexCount;

		UINT bufferSize = sizeof(HStructuredBuffer_Raytracing_ComputeShader_PerModel);

		SharedGraphicsResource graphicResource =
			m_pGraphicMemory->AllocateConstant(bufferSize);

		memcpy(graphicResource.Memory(),
			&perModel,
			bufferSize);

		pModel->structuredBuffer_computeShader_model.Reset(graphicResource);
	}
}

void HModelManager::UpdateVertexBuffer_raytracing(ID3D12GraphicsCommandList* cmdList)
{
	unsigned int currentFrameIndex = m_pDeviceResources->GetCurrentFrameIndex();

	if (m_pAllInstacedVertices.Get() == nullptr)
		return;

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pAllInstacedVertices.Get(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cmdList->ResourceBarrier(1, &barrier);

	cmdList->SetPipelineState(m_pComputePSO.Get());

	cmdList->SetComputeRootSignature(m_pComputeShaderRootSignature.Get());

	cmdList->SetComputeRootShaderResourceView((UINT)RootSig_Compute::StructuredBuffer_VertexInput,
		m_allVerticesResource->GetGPUVirtualAddress());

	cmdList->SetComputeRootUnorderedAccessView((UINT)RootSig_Compute::StructuredBuffer_VertexOutput,
		m_pAllInstacedVertices->GetGPUVirtualAddress());

	//버텍스 숫자 모델마다 유동적
	//모델마다 인스턴스 숫자 유동적
	//(전체 버텍스 숫자/256)
	for (auto& e : m_models)
	{
		HModel* pHModel = e.second.get();


		if (pHModel->rawData.boneCount == 0)
			continue;

		UINT numGroupsX = (UINT)ceilf(pHModel->rawData.vertexData.size() / 256.0f);

		UINT numGroupsY = 0;

		for (auto& k : pHModel->visibleInstances)
		{
			/*std::unordered_map<void*, std::shared_ptr<HInstance>>& instanceList
				= k.second;*/

			std::vector<HInstance*>& instanceList = k.second;

			numGroupsY += instanceList.size();
		}

		if (numGroupsY == 0)
			continue;

		cmdList->SetComputeRootShaderResourceView((UINT)RootSig_Compute::StructuredBuffer_VertexInput,
			pHModel->vertexBuffer->GetGPUVirtualAddress());

		cmdList->SetComputeRootConstantBufferView((UINT)RootSig_Compute::Constant_PerModel,
			pHModel->structuredBuffer_computeShader_model.GpuAddress());

		cmdList->SetComputeRootShaderResourceView((UINT)RootSig_Compute::StructuredBuffer_BoneTM,
			pHModel->structuredBuffer_computeShader_boneTMs.GpuAddress());

		//cmdList->SetComputeRootShaderResourceView((UINT)RootSig_Compute::StructuredBuffer_BoneTM,
		//	pHModel->structuredBuffer_bones[0].GpuAddress());



		cmdList->Dispatch(numGroupsX, numGroupsY, 1);

	}

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pAllInstacedVertices.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cmdList->ResourceBarrier(1, &barrier);

}

void HModelManager::UpdatePerModelResource_raytracing()
{
	std::vector<HSturecturedBuffer_Raytracing_PerModel> perModel;
	perModel.reserve(m_models.size());


	UINT dynamicModelInstanceOffset = 0;

	for (auto& model : m_models)
	{
		HModel* pModel = model.second.get();

		HSturecturedBuffer_Raytracing_PerModel raytracingStructuredBuffer;
		raytracingStructuredBuffer.indexOffsetInEntireBuffer = pModel->indexOffsetInEntireBuffer;

		raytracingStructuredBuffer.vertexCount = pModel->rawData.vertexData.size();
		if (pModel->rawData.boneCount == 0)
		{
			raytracingStructuredBuffer.vertexOffsetInEntireBuffer = pModel->vertexOffsetInEntireBuffer;
			raytracingStructuredBuffer.isDynamicModel = FALSE;
		}
		else
		{
			raytracingStructuredBuffer.vertexOffsetInEntireBuffer = dynamicModelInstanceOffset;
			raytracingStructuredBuffer.isDynamicModel = TRUE;
			dynamicModelInstanceOffset += pModel->rawData.vertexData.size() * pModel->visibleInstanceCount_temp;
		}

		perModel.push_back(raytracingStructuredBuffer);


	}

	if (perModel.size() != 0)
	{
		UINT bufferSizePerModel = sizeof(HSturecturedBuffer_Raytracing_PerModel);

		SharedGraphicsResource graphicResource =
			m_pGraphicMemory->Allocate(perModel.size() * bufferSizePerModel);

		memcpy(graphicResource.Memory(),
			perModel.data(),
			perModel.size() * bufferSizePerModel);

		m_raytracingStructuredBuffer_perModel.Reset(graphicResource);
	}
}

void HModelManager::UpdatePerNodeResource_raytracing()
{
	std::vector<HSturecturedBuffer_Raytracing_PerNode> perNode;
	///reserve code 추가해야함
	for (auto& model : m_models)
	{
		HModel* pModel = model.second.get();

		for (int i = 0; i < pModel->rawData.pDepthZeroMeshNodes.size(); i++)
			UpdatePerNodeResourceRecusively_raytracing(pModel->rawData.pDepthZeroMeshNodes[i], pModel->rawData.AxisSystempModify, perNode);
	}

	if (perNode.size() != 0)
	{
		UINT bufferSizePerNode = sizeof(HSturecturedBuffer_Raytracing_PerNode);

		SharedGraphicsResource graphicResource =
			m_pGraphicMemory->Allocate(perNode.size() * bufferSizePerNode);

		memcpy(graphicResource.Memory(),
			perNode.data(),
			perNode.size() * bufferSizePerNode);

	}

}

void HModelManager::UpdatePerNodeResourceRecusively_raytracing(HMesh* pNode, Matrix AxisChange, std::vector<HSturecturedBuffer_Raytracing_PerNode>& perNode)
{
	//if (pNode->pParentNode != nullptr)
	{
		HSturecturedBuffer_Raytracing_PerNode perNodedata;

		//perNodedata.globalTransform = (pNode->staticTM* AxisChange).Transpose();
		perNodedata.indexOffsetInModel = pNode->indexOffset;
		perNodedata.vertexOffsetInModel = pNode->vertexOffset;
		perNode.push_back(perNodedata);
	}

	for (int i = 0; i < pNode->pChildNode.size(); i++)
	{
		UpdatePerNodeResourceRecusively_raytracing(pNode->pChildNode[i], AxisChange, perNode);
	}

}

void HModelManager::UpdateRaytracingBottomLevelAccelerationStructure_raytracing(ID3D12GraphicsCommandList4* cmdList)
{
	unsigned int currentFrame = m_pDeviceResources->GetCurrentFrameIndex();

	if (m_pAllInstacedVertices.Get() == nullptr)
		return;

	D3D12_GPU_VIRTUAL_ADDRESS dynamicVertexOffset = dynamicVertexOffset
		= m_pAllInstacedVertices->GetGPUVirtualAddress();

	for (auto& model : m_models)
	{
		HModel* pModel = model.second.get();
		if (pModel->rawData.boneCount == 0)
			continue;


		for (unsigned char flag = 0; flag <= ALLOPTION_MM; flag++)
		{
			for (auto& instance : pModel->visibleInstances[flag])
			{
				HInstance* pHInstance = instance;

				D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC triangleDesc;

				triangleDesc.VertexBuffer.StartAddress = dynamicVertexOffset;

				dynamicVertexOffset += pModel->rawData.vertexData.size() * sizeof(HVertex_raytracing);

				triangleDesc.VertexBuffer.StrideInBytes = sizeof(HVertex_raytracing);
				triangleDesc.VertexCount = pModel->rawData.vertexData.size();
				triangleDesc.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

				triangleDesc.IndexBuffer = pModel->indexBuffer->GetGPUVirtualAddress();
				triangleDesc.IndexFormat = DXGI_FORMAT_R32_UINT;
				triangleDesc.IndexCount = pModel->rawData.indexData.size();

				triangleDesc.Transform3x4 = 0;

				D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;

				geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
				geometryDesc.Triangles = triangleDesc;
				geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;


				//	CreateBLAS(cmdList, pHInstance, geometryDesc);

				if (pHInstance->bottomLevelAccelerationStructureBuffer.Get() == nullptr)
					CreateBLAS(cmdList, pHInstance, geometryDesc);
				else
					UpdateBLAS(cmdList, pHInstance, geometryDesc);
			}
		}
	}
}

void HModelManager::CreateBLAS(ID3D12GraphicsCommandList4* cmdList, HInstance* pHInstance, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc)
{
	unsigned int currentFrame = m_pDeviceResources->GetCurrentFrameIndex();
	ID3D12Device5* pDevice = m_pDeviceResources->GetRaytracingDevice();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	bottomLevelInputs.NumDescs = 1;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.pGeometryDescs = &geometryDesc;


	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild;
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &prebuild);

	CD3DX12_HEAP_PROPERTIES defaultHeapProp(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC scratchResourceDesc =
		CD3DX12_RESOURCE_DESC::Buffer(prebuild.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	CD3DX12_RESOURCE_DESC resultResourceDesc =
		CD3DX12_RESOURCE_DESC::Buffer(prebuild.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	HBufferManager* pBufferManager = HBufferManager::GetInstance();

	pBufferManager->CreateDefaultHeapBuffer_device5(pDevice, scratchResourceDesc, D3D12_RESOURCE_STATE_COMMON,
		pHInstance->bottomLevelAccelerationStructureScratchBuffer);

	pBufferManager->CreateDefaultHeapBuffer_device5(pDevice, resultResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		pHInstance->bottomLevelAccelerationStructureBuffer);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	bottomLevelBuildDesc.Inputs = bottomLevelInputs;
	bottomLevelBuildDesc.ScratchAccelerationStructureData =
		pHInstance->bottomLevelAccelerationStructureScratchBuffer->GetGPUVirtualAddress();
	bottomLevelBuildDesc.DestAccelerationStructureData =
		pHInstance->bottomLevelAccelerationStructureBuffer->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);


	// Wait for the builder to complete by setting a barrier on the resulting
	// buffer. This can be important in case the rendering is triggered
	// immediately afterwards, without executing the command list
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pHInstance->bottomLevelAccelerationStructureBuffer.Get();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	cmdList->ResourceBarrier(1, &uavBarrier);


}

void HModelManager::UpdateBLAS(ID3D12GraphicsCommandList4* cmdList, HInstance* pHInstance, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc)
{
	unsigned int currentFrame = m_pDeviceResources->GetCurrentFrameIndex();
	ID3D12Device5* pDevice = m_pDeviceResources->GetRaytracingDevice();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	bottomLevelInputs.NumDescs = 1;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.pGeometryDescs = &geometryDesc;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	bottomLevelBuildDesc.Inputs = bottomLevelInputs;
	bottomLevelBuildDesc.SourceAccelerationStructureData =
		pHInstance->bottomLevelAccelerationStructureBuffer->GetGPUVirtualAddress();
	bottomLevelBuildDesc.ScratchAccelerationStructureData =
		pHInstance->bottomLevelAccelerationStructureScratchBuffer->GetGPUVirtualAddress();
	bottomLevelBuildDesc.DestAccelerationStructureData =
		pHInstance->bottomLevelAccelerationStructureBuffer->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);


	// Wait for the builder to complete by setting a barrier on the resulting
	// buffer. This can be important in case the rendering is triggered
	// immediately afterwards, without executing the command list
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pHInstance->bottomLevelAccelerationStructureBuffer.Get();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	cmdList->ResourceBarrier(1, &uavBarrier);

}

void HModelManager::UpdateRaytracingTopLevelAccelerationStructure_raytracing(ID3D12GraphicsCommandList4* cmdList)
{
	int currentFrame = m_pDeviceResources->GetCurrentFrameIndex();
	auto device = m_pDeviceResources->GetRaytracingDevice();

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
	instanceDescs.reserve(m_instances.size());
	int instanceIndex = 0;

	for (auto& model : m_models)
	{
		HModel* pModel = model.second.get();

		for (unsigned char flag = 0; flag <= ALLOPTION_MM; flag++)
		{
			for (auto& instance : pModel->visibleInstances[flag])
			{
				HInstance* pHInstance = instance;

				Matrix worldTm = (pModel->rawData.AxisSystempModify * pHInstance->worldTM).Transpose();

				D3D12_RAYTRACING_INSTANCE_DESC rtInstance;

				rtInstance =
				{
				{{worldTm._11,worldTm._12,worldTm._13,worldTm._14},						//Transform[3][4]
				{worldTm._21,worldTm._22,worldTm._23,worldTm._24},
				{worldTm._31,worldTm._32,worldTm._33,worldTm._34}},
				//래스터라이즈 렌더링에서의 모델의 WorldTM
				(UINT)instanceIndex,													//InstanceID : 24
																						//히트 쉐이더를 로드할 때 인스턴스당 쉐이더를 분리하는데 쓰임
				0xFF,																	//InstanceMask :8
																						//TraceRay()실행 시 특정 인스턴스를 분리하는데 쓰인다. TraceRay()에서
																						//전달한 마스크와 해당 인스턴스 마스크의 일치 비트가 있을 경우 해당 인스턴스는
																						//TraceRay()에서 식별가능(Visible)하다
				0,																		//InstanceContributionToHitGroupIndex : 24
																						//인스턴스의 히트쉐이더 인덱스 추가값을 설정한다. 인스턴스 ID와 마찬가지로,
																						//히트쉐이더를 로드할 때 인스턴스당 쉐이더를 분리하는데 쓰인다.
				D3D12_RAYTRACING_INSTANCE_FLAG_NONE,									//Flags : 8
																						//추가 옵션, 대부분의 경우 NONE
				pModel->bottomLevelAccelerationStructureBuffer->GetGPUVirtualAddress()	//AccelerationStructure
				};

				if (pModel->rawData.boneCount != 0)
				{
					rtInstance.AccelerationStructure =
						pHInstance->bottomLevelAccelerationStructureBuffer->GetGPUVirtualAddress();
				}

				instanceDescs.push_back(rtInstance);
				instanceIndex++;
			}
		}
	}

	if (instanceDescs.size() != 0)
	{
		SharedGraphicsResource graphicResourceCB =
			m_pGraphicMemory->Allocate(instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

		memcpy(graphicResourceCB.Memory(),
			instanceDescs.data(),
			instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

		m_pRaytracingInstanceDescs[currentFrame].Reset(graphicResourceCB);
	}

	//레이트레이싱 인스턴스 입력 정보를 작성
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};

	if (instanceDescs.size() != 0)
	{
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		inputs.NumDescs = instanceDescs.size();
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.InstanceDescs = m_pRaytracingInstanceDescs[currentFrame].GpuAddress();
	}

	//가속화 구조체의 필요 크기를 조회
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);

	CD3DX12_HEAP_PROPERTIES defaultHeapProp(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC scratchResourceDesc =
		CD3DX12_RESOURCE_DESC::Buffer(prebuild.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	CD3DX12_RESOURCE_DESC resultResourceDesc =
		CD3DX12_RESOURCE_DESC::Buffer(prebuild.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	device->CreateCommittedResource(&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&scratchResourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(m_pTopLevelAccelerationStructureScratches[currentFrame].ReleaseAndGetAddressOf()));

	device->CreateCommittedResource(&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resultResourceDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(m_pTopLevelAccelerationStructures[currentFrame].ReleaseAndGetAddressOf()));

	//CommandList 인터페이스를 이용해서 가속화 구조체를 작성
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.DestAccelerationStructureData = m_pTopLevelAccelerationStructures[currentFrame]->GetGPUVirtualAddress();
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = m_pTopLevelAccelerationStructureScratches[currentFrame]->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// Wait for the builder to complete by setting a barrier on the resulting
	// buffer. This can be important in case the rendering is triggered
	// immediately afterwards, without executing the command list
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_pTopLevelAccelerationStructures[currentFrame].Get();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	cmdList->ResourceBarrier(1, &uavBarrier);
}


void HModelManager::CreateComputePSO_lightCalculation()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {};
	computePipelineStateDesc.pRootSignature = m_pComputeShaderRootSignature.Get();
	computePipelineStateDesc.CS =
	{
		g_pSkinAnim_Raytracing,
		sizeof(g_pSkinAnim_Raytracing)
	};
	computePipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	DX::ThrowIfFailed(device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&m_pComputePSO)));
}

void HModelManager::CreateComputeRootSignature()
{
	HRESULT hr = S_OK;

	CD3DX12_ROOT_PARAMETER rootParameters[(UINT)RootSig_Compute::MaxCount];
	rootParameters[(UINT)RootSig_Compute::StructuredBuffer_VertexOutput].InitAsUnorderedAccessView(0);
	rootParameters[(UINT)RootSig_Compute::StructuredBuffer_VertexInput].InitAsShaderResourceView(0);
	rootParameters[(UINT)RootSig_Compute::StructuredBuffer_BoneTM].InitAsShaderResourceView(1, 1);
	rootParameters[(UINT)RootSig_Compute::Constant_PerModel].InitAsConstantBufferView(0);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(ARRAYSIZE(rootParameters), rootParameters);

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
		IID_PPV_ARGS(&m_pComputeShaderRootSignature)));
}





