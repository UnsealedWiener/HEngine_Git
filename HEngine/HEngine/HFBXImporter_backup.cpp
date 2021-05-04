#include"pch.h"
#include"HFbxUtil.h"
#include"HFBXImporter.h"



HFbxImporter::HFbxImporter()
{
	m_pFbxManager = FbxManager::Create();

	// Create the IO settings object.
	m_pIOSettings = FbxIOSettings::Create(m_pFbxManager, IOSROOT);
	m_pFbxManager->SetIOSettings(m_pIOSettings);

	// Create an importer using the SDK manager.
	m_pImporter = FbxImporter::Create(m_pFbxManager, "");
}

HFbxImporter::~HFbxImporter()
{
	m_pIOSettings->Destroy();
	m_pImporter->Destroy();
	m_pFbxManager->Destroy();
}

void HFbxImporter::ImportFbx(const char* filename)
{
	// Use the first argument as the filename for the importer.
	if (!m_pImporter->Initialize(filename, -1, m_pFbxManager->GetIOSettings()))
	{
		HerrorMessage(L"The model fbx file is not found")
			throw std::exception("fbxFile not found");
	}

	// Create a new scene so that it can be populated by the imported file.
	FbxScene* pScene = FbxScene::Create(m_pFbxManager, filename);

	// Import the contents of the file into the scene.
	m_pImporter->Import(pScene);

	//fbx의 좌표계 정보를 불러오고 이를 기반으로 directX 좌표계로의 변환 matrix 생성  

	
	m_axisSystem = pScene->GetGlobalSettings().GetAxisSystem();
	CalAxisMaxToDirectX(m_axisSystem);
	
	//만약 점 정보들이 삼각형 기반이 아니라면 삼각형으로 만들어준다.
	FbxGeometryConverter converter(m_pFbxManager);
	converter.Triangulate(pScene, true);

	ProcessScene(pScene);

	pScene->Destroy();

	GenerateNodeData();
}

void HFbxImporter::ImportAnimation(const char* filename)
{
	if (!m_pImporter->Initialize(filename, -1, m_pFbxManager->GetIOSettings()))
	{
		HerrorMessage(L"fbxFile animation file is not found.");
		throw std::exception("fbxFile not found.");
	}

	// Create a new scene so that it can be populated by the imported file.
	FbxScene* pScene = FbxScene::Create(m_pFbxManager, filename);

	// Import the contents of the file into the scene.
	m_pImporter->Import(pScene);

	//if (m_axisSystem != pScene->GetGlobalSettings().GetAxisSystem())
	//{
	//	HerrorMessage(L"the animation's axisSystem doesn't match the model.");
	//	throw std::exception("the animation's axisSystem doesn't match the model.");
	//}

	std::string fileName = filename;
	std::string animName = fileName.substr(fileName.find_last_of("@") + 1, fileName.find_last_of(".fbx") - fileName.find_last_of("@") - 4);

	ProcessAnimations(pScene, animName);

	pScene->Destroy();
}

void HFbxImporter::CalAxisMaxToDirectX(FbxAxisSystem maxAxisSystem)
{
	FbxVector4	vScale(1.0, 1.0, -1.0, 1);
	FbxVector4	vRotation(90, 0.0, 0.0, 1);
	FbxVector4	vTranslation(0.0, 0.0, 0.0, 1);

	FbxAMatrix rot(FbxVector4(0.0, 0.0, 0.0), vRotation, FbxVector4(1.0, 1.0, 1.0));
	FbxAMatrix scale(FbxVector4(0.0, 0.0, 0.0), FbxVector4(0.0, 0.0, 0.0), vScale);

	FbxAMatrix rootTM;

	int direction = 1;

	//3d max내의 export 옵션 중 좌표계 선택이 있다. yUp 또는 zUp을 선택할 수 있다.
	if (maxAxisSystem.GetUpVector(direction) == FbxAxisSystem::eYAxis)
	{
		rootTM = scale;
	}
	else if (maxAxisSystem.GetUpVector(direction) == FbxAxisSystem::eZAxis)
	{
		rootTM = rot * scale;
	}

	m_axisChange = ConvertMatrix(rootTM);
}

void HFbxImporter::ProcessScene(FbxScene* pScene)
{
	FbxNode* fbxRootNode = pScene->GetRootNode();

	ProcessNodesRecursively(fbxRootNode, nullptr, fbxsdk::FbxNodeAttribute::eSkeleton);
	ProcessNodesRecursively(fbxRootNode, nullptr, fbxsdk::FbxNodeAttribute::eMesh);
}

void HFbxImporter::ProcessNodesRecursively(FbxNode* pFbxNode, NodeData* pParent, FbxNodeAttribute::EType currNodeType)
{
	NodeData* pNode = nullptr;

	FbxNodeAttribute* attribute = pFbxNode->GetNodeAttribute();

	if (attribute != nullptr)
	{
		FbxNodeAttribute::EType nodeType = attribute->GetAttributeType();

		if (nodeType == currNodeType)
		{

			switch (nodeType)
			{
			case fbxsdk::FbxNodeAttribute::eSkeleton:
				pNode = new BoneData;
				m_totalBoneCount++;
				ProcessBones(pFbxNode, (BoneData*)pNode);
				m_boneData.push_back((BoneData*)pNode);
				if (pParent == nullptr)
					m_boneData_parents.push_back((BoneData*)pNode);
 				break;
			case fbxsdk::FbxNodeAttribute::eMesh:
				pNode = new MeshData;

				ProcessMesh(pFbxNode, (MeshData*)pNode);
				if (pParent == nullptr)
				{
					m_meshData.push_back((MeshData*)pNode);
				}
				break;

			default:
				break;
			}

			pNode->nodeName = pFbxNode->GetName();
			pNode->typeName = GetAttributeTypeName(nodeType);
			pNode->pParentNode = pParent;

			if (pParent != nullptr)
			{
				pParent->pChildNodes.push_back(pNode);
			}
		}
	}

	for (int i = 0; i < pFbxNode->GetChildCount(); i++)
	{
		ProcessNodesRecursively(pFbxNode->GetChild(i), pNode, currNodeType);
	}
}

void HFbxImporter::ProcessBones(FbxNode* pFbxNode, BoneData* pNode)
{
	// FbxSkeleton* pSkeleton = pFbxNode->GetSkeleton();
	pNode->boneIndex = m_boneData.size();
	pNode->localTTM = ConvertMatrix(pFbxNode->EvaluateLocalTransform());
	//pNode->invGlobalTTM = ConvertMatrix(pFbxNode->EvaluateGlobalTransform().Inverse());
}


void HFbxImporter::ProcessMesh(FbxNode* pNode, MeshData* pNodeOutput)
{
	FbxMesh* pMesh = pNode->GetMesh();

	int triangleCount = pMesh->GetPolygonCount();
	int vertexCounter = 0;
	int polygonSize = 3;
	int controlPointCount = pMesh->GetControlPointsCount();
	std::vector<HVertex> vertices;
	vertices.resize(triangleCount * 3);

	std::vector<BoneIndexWeight> boneIndexWeight(controlPointCount);

	CreateBoneWeight(pMesh, boneIndexWeight, pNodeOutput);
	ShrinkBoneNumberToFour(boneIndexWeight);

	for (unsigned int i = 0; i < triangleCount; i++)
	{
		for (int j = 0; j < polygonSize; j++, vertexCounter++)
		{
			HVertex vertex = {};

			int ctrlPointIndex = pMesh->GetPolygonVertex(i, j);

			ReadPosition(pMesh, ctrlPointIndex, vertex);
			ReadTangentBasis(pMesh, ctrlPointIndex, vertexCounter, vertex);
			ReadUV(pMesh, ctrlPointIndex, vertexCounter, vertex);
			ReadBoneWeight(boneIndexWeight, ctrlPointIndex, vertex);
			//오른손좌표계->왼손좌표계 이동시 winding순서를 바꿔줘야함. 따라서 버텍스 정보를 역으로 넣어줌.
			vertices[triangleCount * 3 - 1 - vertexCounter] = vertex;
			//vertices.push_back(vertex);
		}
	}

	pNodeOutput->vertices = vertices;
	pNodeOutput->staticTM = ConvertMatrix(pNode->EvaluateGlobalTransform());

	FbxVector4 boxMin;
	FbxVector4 boxMax;
	FbxVector4 boxCenter;

	pNode->EvaluateGlobalBoundingBoxMinMaxCenter(boxMin, boxMax, boxCenter);

	XMFLOAT4 dxBoxMin = ConvertFloat4(boxMin);
	XMFLOAT4 dxBoxMax = ConvertFloat4(boxMax);

	XMVECTOR simdBoxMin = XMLoadFloat4(&dxBoxMin);
	XMVECTOR simdBoxMax = XMLoadFloat4(&dxBoxMax);


	BoundingBox::CreateFromPoints(pNodeOutput->boundingBox, simdBoxMin
		, simdBoxMax);
}

void HFbxImporter::ReadPosition(FbxMesh* inMesh, int inCtrlPointIndex, HVertex& vertex)
{
	vertex.pos = ConvertFloat3(inMesh->GetControlPointAt(inCtrlPointIndex));
}

void HFbxImporter::ReadTangentBasis(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, HVertex& vertex)
{

	if (inMesh->GetElementNormalCount() < 1) { throw std::exception("Invalid Normal Number"); }

	FbxGeometryElementNormal* normal = inMesh->GetElementNormal();
	FbxGeometryElementBinormal* biNormal = inMesh->GetElementBinormal();
	FbxGeometryElementTangent* tangent = inMesh->GetElementTangent();

	switch (normal->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (normal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			vertex.normal = ConvertFloat3(normal->GetDirectArray().GetAt(inCtrlPointIndex));
			vertex.biNormV = ConvertFloat3(biNormal->GetDirectArray().GetAt(inCtrlPointIndex));
			vertex.tangentU = ConvertFloat3(tangent->GetDirectArray().GetAt(inCtrlPointIndex));
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = normal->GetIndexArray().GetAt(inCtrlPointIndex);
			vertex.normal = ConvertFloat3(normal->GetDirectArray().GetAt(index));
			vertex.biNormV = ConvertFloat3(biNormal->GetDirectArray().GetAt(index));
			vertex.tangentU = ConvertFloat3(tangent->GetDirectArray().GetAt(index));
		}
		break;
		default:
			throw std::exception("Invalid Reference");
		}
		break;
	case FbxGeometryElement::eByPolygonVertex:
		switch (normal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			vertex.normal = ConvertFloat3(normal->GetDirectArray().GetAt(inVertexCounter));
			vertex.biNormV = ConvertFloat3(biNormal->GetDirectArray().GetAt(inVertexCounter));
			vertex.tangentU = ConvertFloat3(tangent->GetDirectArray().GetAt(inVertexCounter));
		}
		break;
		case FbxGeometryElement::eIndexToDirect:
		{
			int index = normal->GetIndexArray().GetAt(inVertexCounter);
			vertex.normal = ConvertFloat3(normal->GetDirectArray().GetAt(index));
			vertex.biNormV = ConvertFloat3(biNormal->GetDirectArray().GetAt(index));
			vertex.tangentU = ConvertFloat3(tangent->GetDirectArray().GetAt(index));
		}
		break;
		default: throw std::exception("Invalid Reference");
		}

		break;
	}

}

void HFbxImporter::ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, HVertex& vertex)
{
	if (inMesh->GetElementUVCount() < 1) { throw std::exception("Invalid UV Number"); }

	FbxGeometryElementUV* UV = inMesh->GetElementUV();

	switch (UV->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (UV->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			vertex.texC = ConvertFloat2(UV->GetDirectArray().GetAt(inCtrlPointIndex));
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = UV->GetIndexArray().GetAt(inCtrlPointIndex);
			vertex.texC = ConvertFloat2(UV->GetDirectArray().GetAt(index));
		}
		break;
		default:
			throw std::exception("Invalid Reference");
		}
		break;
	case FbxGeometryElement::eByPolygonVertex:
		switch (UV->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			vertex.texC = ConvertFloat2(UV->GetDirectArray().GetAt(inVertexCounter));
		}
		break;
		case FbxGeometryElement::eIndexToDirect:
		{
			int index = UV->GetIndexArray().GetAt(inVertexCounter);
			vertex.texC = ConvertFloat2(UV->GetDirectArray().GetAt(index));
		}
		break;
		default: throw std::exception("Invalid Reference");
		}
		break;
	}

	vertex.texC.y = 1 - vertex.texC.y;
}

void HFbxImporter::ReadBoneWeight(std::vector<BoneIndexWeight>& boneIndexWeight, int ctrlPoint, HVertex& vertex)
{
	BoneIndexWeight currBoneIndexWeight = boneIndexWeight[ctrlPoint];

	vertex.boneIndex.x = currBoneIndexWeight.boneIndexWeights[0].first;
	vertex.blendWeight.x = currBoneIndexWeight.boneIndexWeights[0].second;

	vertex.boneIndex.y = currBoneIndexWeight.boneIndexWeights[1].first;
	vertex.blendWeight.y = currBoneIndexWeight.boneIndexWeights[1].second;

	vertex.boneIndex.z = currBoneIndexWeight.boneIndexWeights[2].first;
	vertex.blendWeight.z = currBoneIndexWeight.boneIndexWeights[2].second;

	vertex.boneIndex.w = currBoneIndexWeight.boneIndexWeights[3].first;
	vertex.blendWeight.w = currBoneIndexWeight.boneIndexWeights[3].second;

	//for (int i = 0; i < currBoneIndexWeight.boneIndexWeights.size(); i++)
	//{

	//	switch (i)
	//	{
	//	case 0:
	//		vertex.boneIndex.x = currBoneIndexWeight.boneIndexWeights[i].first;
	//		vertex.blendWeight.x = currBoneIndexWeight.boneIndexWeights[i].second;
	//		break;
	//	case 1:
	//		vertex.boneIndex.y = currBoneIndexWeight.boneIndexWeights[i].first;
	//		vertex.blendWeight.y = currBoneIndexWeight.boneIndexWeights[i].second;
	//		break;
	//	case 2:
	//		vertex.boneIndex.z = currBoneIndexWeight.boneIndexWeights[i].first;
	//		vertex.blendWeight.z = currBoneIndexWeight.boneIndexWeights[i].second;
	//		break;
	//	case 3:
	//		vertex.boneIndex.w = currBoneIndexWeight.boneIndexWeights[i].first;
	//		vertex.blendWeight.w = currBoneIndexWeight.boneIndexWeights[i].second;
	//		break;
	//	default:
	//		break;
	//	}
	//}
}

void HFbxImporter::CreateBoneWeight(FbxMesh* inMesh, std::vector<BoneIndexWeight>& boneIndexWeight, MeshData* pNodeOutput)
{
	if (inMesh->GetDeformerCount() != 0)
	{
		int deformerCount = inMesh->GetDeformerCount();

		for (int i = 0; i < inMesh->GetDeformerCount(); i++)
		{
			FbxDeformer* pDeformer = inMesh->GetDeformer(i);

			if (pDeformer->GetDeformerType() == FbxDeformer::eSkin)
			{
				FbxSkin* pSkin = (FbxSkin*)pDeformer;
				int clusterCount = pSkin->GetClusterCount();

				for (int j = 0; j < pSkin->GetClusterCount(); j++)
				{
					FbxCluster* pCluster = pSkin->GetCluster(j);

					FbxCluster::ELinkMode linkMode = pCluster->GetLinkMode();

					FbxNode* pLinkNode = pCluster->GetLink();

					std::string linkNodeName = pLinkNode->GetName();

					FbxAMatrix transformMatrix;
					FbxAMatrix transformLinkMatrix;

					pCluster->GetTransformMatrix(transformMatrix);//모든 cluster들이 같은 TM을 리턴한다고 한다.
					pCluster->GetTransformLinkMatrix(transformLinkMatrix);///나중에 다시 확인※

					pNodeOutput->globalBindTM = ConvertMatrix(transformMatrix);
					pNodeOutput->hasDeformingBone = true;

					int BoneIndex = FindBoneIndex(linkNodeName);
					if (BoneIndex == -1)
						throw std::exception("bone index not found");


					
					m_boneData[BoneIndex]->invGlobalTTM = ConvertMatrix(transformLinkMatrix.Inverse());
					

					int* indices = pCluster->GetControlPointIndices();
					double* weights = pCluster->GetControlPointWeights();

					for (int k = 0; k < pCluster->GetControlPointIndicesCount(); k++)
					{
						std::pair<unsigned int, float> currIndexWeight(BoneIndex, weights[k]);
						boneIndexWeight[indices[k]].boneIndexWeights.push_back(currIndexWeight);
					}
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < boneIndexWeight.size(); i++)
		{
			std::pair<unsigned int, float> currIndexWeight(0, 0);
			boneIndexWeight[i].boneIndexWeights.push_back(currIndexWeight);
		}
	}

}

void HFbxImporter::ShrinkBoneNumberToFour(std::vector<BoneIndexWeight>& boneIndexWeight)
{
	for (int i = 0; i < boneIndexWeight.size(); i++)
	{
		if (boneIndexWeight[i].boneIndexWeights.size() > 4)
		{
			BubbleSort(boneIndexWeight[i].boneIndexWeights);

			boneIndexWeight[i].boneIndexWeights[3].second =
				1 - (boneIndexWeight[i].boneIndexWeights[0].second +
					boneIndexWeight[i].boneIndexWeights[1].second +
					boneIndexWeight[i].boneIndexWeights[2].second);
		}

		if (boneIndexWeight[i].boneIndexWeights.size() < 4)
		{
			int emptyBoneCnt = 4 - boneIndexWeight[i].boneIndexWeights.size();
			for (int j = 0; j < emptyBoneCnt; j++)
			{
				boneIndexWeight[i].boneIndexWeights.push_back(std::make_pair<unsigned int, float>(0, 0));
			}
		}
	}


}

void HFbxImporter::BubbleSort(std::vector<std::pair<unsigned int, float>> &boneIndexWeights)
{

	for (int j = 0; j < boneIndexWeights.size() - 1; j++)
	{
		for (int i = 1; i < boneIndexWeights.size() - j; i++)
		{
			if (boneIndexWeights[i - 1].second < boneIndexWeights[i].second)
				std::swap(boneIndexWeights[i - 1], boneIndexWeights[i]);
		}

	}
}

int HFbxImporter::FindBoneIndex(std::string boneName)
{
	int boneIndex = -1;
	for (BoneData* Bone : m_boneData)
	{
		if (Bone->nodeName == boneName)
		{
			boneIndex = Bone->boneIndex;
			break;
		}
	}
	return boneIndex;
}



void HFbxImporter::ProcessAnimations(FbxScene* pScene, std::string& animName)
{
	//초당 프레임 수(fbx 기준)
	float framesPerSecond = (float)FbxTime::GetFrameRate(pScene->GetGlobalSettings().GetTimeMode());

	int numStacks = pScene->GetSrcObjectCount<FbxAnimStack>();

	FbxAnimStack* pAnimStack = FbxCast<FbxAnimStack>(pScene->GetSrcObject<FbxAnimStack>(0));

	//원래 fbx내부에서 정해진 애니메이션 이름으로 하려고 했으나 명확성을 위해서 파일 이름 기준으로 바꾼다.
	//std::string name = pAnimStack->GetName();

	FbxTimeSpan timeSpan = pAnimStack->GetLocalTimeSpan();
	double startTime = timeSpan.GetStart().GetSecondDouble();
	double endTime = timeSpan.GetStop().GetSecondDouble();

	AnimationData* pAnimData = new AnimationData;
	pAnimData->framesPerSecond = framesPerSecond;
	pAnimData->totalFrames = (int)((endTime - startTime) * (double)framesPerSecond);
	pAnimData->startTime = startTime;
	pAnimData->endTime = endTime;
	pAnimData->totalTime = endTime - startTime;
	pAnimData->name = animName.c_str();

	pAnimData->allBoneAnim.reserve(m_boneData.size());

	FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>();

	ProcessNodesRecursively_anim(pAnimLayer, pScene->GetRootNode(), nullptr, pAnimData);

	//if (CheckIfAnimAndModelMatch(pAnimData) != true)
	//{
	//	HerrorMessage(L"model fbx and anim fbx mismatch")
	//		throw std::exception("The animation data doesn't match model data.");
	//}

	m_animData.push_back(pAnimData);

	//여러  애니메이션 레이어가 존재할 때 사용

   // int numAnimLayers = pAnimStack->GetMemberCount<FbxAnimLayer>();

   // for (int j = 0; j < numAnimLayers; j++)
   // {
	FbxAnimLayer* lAnimLayer = pAnimStack->GetMember<FbxAnimLayer>();
	// }


//원래 하나의 fbx에 여러 애니메이션 데이터를 담으려고 했는데 명확함을 위해 하나씩만 담는 구조로 바꾼다.

/*
for (int i = 0; i < numStacks; i++)
{
	FbxAnimStack* pAnimStack = FbxCast<FbxAnimStack>(pScene->GetSrcObject<FbxAnimStack>(i));

	std::string name = pAnimStack->GetName();

	FbxTimeSpan timeSpan = pAnimStack->GetLocalTimeSpan();
	double startTime = timeSpan.GetStart().GetSecondDouble();
	double endTime = timeSpan.GetStop().GetSecondDouble();

	AnimationData* pAnimData = new AnimationData;
	pAnimData->framesPerSecond = framesPerSecond;
	pAnimData->totalFrames = (int)((endTime - startTime) * (double)framesPerSecond);
	pAnimData->startTime = startTime;
	pAnimData->endTime = endTime;
	pAnimData->totalTime = endTime - startTime;
	pAnimData->name = name;

	FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>();


	ProcessNodesRecursively_anim_Temp(pAnimLayer, pScene->GetRootNode(), nullptr, pAnimData);

	if (CheckIfAnimAndModelMatch(pAnimData) != true)
		throw std::exception("The animation data doesn't match model data.");

	m_animData.push_back(pAnimData);

	//여러  애니메이션 레이어가 존재할 때 사용

   // int numAnimLayers = pAnimStack->GetMemberCount<FbxAnimLayer>();

   // for (int j = 0; j < numAnimLayers; j++)
   // {
		FbxAnimLayer* lAnimLayer = pAnimStack->GetMember<FbxAnimLayer>();
   // }

}*/

}

bool HFbxImporter::CheckIfAnimAndModelMatch(AnimationData* pAnimData)
{
	if (m_meshData.size() != pAnimData->meshAnim.size())
		return false;

	for (int i = 0; i < m_meshData.size(); i++)
		if (CheckByNameMatchRecursively(m_meshData[i], pAnimData->meshAnim[i]) == false)
			return false;

	if (m_boneData_parents.size() != pAnimData->boneAnim.size())
		return false;

	for (int i = 0; i < m_boneData_parents.size(); i++)
		if (CheckByNameMatchRecursively(m_boneData_parents[i], pAnimData->boneAnim[i]) == false)
			return false;

	return true;
}

bool HFbxImporter::CheckByNameMatchRecursively(NodeData* pMeshOrBone, NodeData* pAnimNode)
{
	if (pMeshOrBone->nodeName != pAnimNode->nodeName)
		return false;

	if (pMeshOrBone->pChildNodes.size() != pAnimNode->pChildNodes.size())
		return false;

	for (int i = 0; i < pMeshOrBone->pChildNodes.size(); i++)
	{
		CheckByNameMatchRecursively(pMeshOrBone->pChildNodes[i], pAnimNode->pChildNodes[i]);
	}

	return true;
}


void HFbxImporter::ProcessNodesRecursively_anim(FbxAnimLayer* pAnimLayer, FbxNode* pFbxNode, NodeAnimData* pParent, AnimationData* pAnimData)
{
	NodeAnimData* pNode = nullptr;

	FbxNodeAttribute* attribute = pFbxNode->GetNodeAttribute();

	if (attribute != nullptr)
	{

		FbxNodeAttribute::EType nodeType = attribute->GetAttributeType();

		if (nodeType == fbxsdk::FbxNodeAttribute::eSkeleton ||
			nodeType == fbxsdk::FbxNodeAttribute::eMesh)
		{
			pNode = new NodeAnimData;
			pNode->nodeName = pFbxNode->GetName();
			pNode->typeName = GetAttributeTypeName(nodeType);

			
			switch (nodeType)
			{
			case fbxsdk::FbxNodeAttribute::eSkeleton:
				ProcessKeyFrames(pAnimLayer, pFbxNode, pNode, pAnimData);
				pAnimData->allBoneAnim.push_back(pNode);
				if (pParent == nullptr)
					pAnimData->boneAnim.push_back(pNode);
				break;
			case fbxsdk::FbxNodeAttribute::eMesh:
				//ReadBindTM(pFbxNode->GetMesh(), pNode);
				if (pParent == nullptr)
					pAnimData->meshAnim.push_back(pNode);
				break;

			default:
				break;
			}


			pNode->pParentNode = pParent;

			if (pParent != nullptr)
			{
				pParent->pChildNodes.push_back(pNode);
			}
		}
	}

	for (int i = 0; i < pFbxNode->GetChildCount(); i++)
	{
		ProcessNodesRecursively_anim(pAnimLayer, pFbxNode->GetChild(i), pNode, pAnimData);
	}
}

void HFbxImporter::ProcessKeyFrames(FbxAnimLayer* pAnimLayer, FbxNode* pFbxNode, NodeAnimData* pNodeAnim, AnimationData* pAnimData)
{
	double startTime = pAnimData->startTime;

	FbxProperty pProperty_localTrans = pFbxNode->LclTranslation;
	
	if (pProperty_localTrans.IsValid())
	{
		FbxAnimCurveNode* pAnimCurveNode = pProperty_localTrans.GetCurveNode(pAnimLayer);

		if (pAnimCurveNode != NULL)
		{
			int curveCount = pAnimCurveNode->GetCurveCount(0);

			for (int i = 0; i < pAnimCurveNode->GetCurveCount(0); i++)
			{
				FbxAnimCurve* animCurve = pAnimCurveNode->GetCurve(0, i);

				if (animCurve != NULL)
				{
					int keyCount = animCurve->KeyGetCount();//키 카운트가 +1되서 나올 때가 있다(그 키 시간값은 음수로 나온다.)
					pNodeAnim->keyFrames.reserve(keyCount);

					for (int j = 0; j < animCurve->KeyGetCount(); j++)
					{
						FbxAnimCurveKey animCurveKey = animCurve->KeyGet(j);
						FbxTime time = animCurveKey.GetTime();
						float timef = (float)time.GetSecondDouble();

						KeyFrameData currKey;

						currKey.time = timef - startTime;

						Matrix TM = ConvertMatrix(pFbxNode->EvaluateLocalTransform(time));

						DirectX::XMVECTOR vScale;
						DirectX::XMVECTOR vQuatRot;
						DirectX::XMVECTOR vTrans;

						DirectX::XMMatrixDecompose(&vScale, &vQuatRot, &vTrans, TM);

						XMStoreFloat3(&currKey.scale, vScale);
						XMStoreFloat4(&currKey.quatRot, vQuatRot);
						XMStoreFloat3(&currKey.trans, vTrans);

						//currKey.globalTM = ConvertMatrix(pFbxNode->EvaluateGlobalTransform(time));

						pNodeAnim->keyFrames.push_back(currKey);
					}
				}
			}
		}
	}

}

void HFbxImporter::CreateFinalVertex(MeshData* pNode)
{
	Matrix nodeToScene;
	if (pNode->hasDeformingBone)
	{
		nodeToScene = pNode->globalBindTM;
	}
	else
	{
		nodeToScene = pNode->staticTM;
	}

	for (int i = 0; i < pNode->vertices.size(); i++)
	{
		pNode->vertices[i].pos = Vector3::Transform(pNode->vertices[i].pos, nodeToScene);
		pNode->vertices[i].normal = Vector3::TransformNormal(pNode->vertices[i].normal, nodeToScene);
		pNode->vertices[i].tangentU = Vector3::TransformNormal(pNode->vertices[i].tangentU, nodeToScene);
		pNode->vertices[i].biNormV = Vector3::TransformNormal(pNode->vertices[i].biNormV, nodeToScene);
	}

	for (int i = 0; i < pNode->pChildNodes.size(); i++)
	{
		CreateFinalVertex((MeshData*)pNode->pChildNodes[i]);
	}
}

void HFbxImporter::GenerateNodeData()
{
	BoundingBox mergedBoundingBox;

	for (int i = 0; i < m_meshData.size(); i++)
	{
		CreateFinalVertex(m_meshData[i]);
		MergeVertexAndMakeIndex(m_meshData[i]);

		if (i == 0)
		{
			mergedBoundingBox = m_meshData[i]->boundingBox;
		}
		else
		{
			MergeBoundingBox(mergedBoundingBox, m_meshData[i]);
		}
	}

	m_boundingBox = mergedBoundingBox;

	m_allIndices.reserve(m_allVertices.size());///임시코드※

	for (std::uint32_t i = 0; i < m_allVertices.size(); i++)
	{
		m_allIndices.push_back(i);
	}
}

void HFbxImporter::MergeVertexAndMakeIndex(MeshData* pNode)
{
	pNode->vertexOffset = m_allVertices.size();
	pNode->indexOffset = m_allIndices.size();///임시코드※

	pNode->vertexCount = pNode->vertices.size();
	pNode->indexCount = pNode->vertices.size();///임시코드※

	m_allVertices.insert(m_allVertices.end(), pNode->vertices.begin(), pNode->vertices.end());

	for (int i = 0; i < pNode->pChildNodes.size(); i++)
	{
		MergeVertexAndMakeIndex((MeshData*)pNode->pChildNodes[i]);
	}
}

void HFbxImporter::MergeBoundingBox(BoundingBox& mergedBoundingBox, MeshData* pNode)
{
	BoundingBox::CreateMerged(mergedBoundingBox, mergedBoundingBox, pNode->boundingBox);
}

FbxAMatrix HFbxImporter::GetGeometryTransformation(FbxNode* pnode)
{
	if (!pnode)
	{
		throw std::exception("Null for mesh geometry");
	}

	const FbxVector4 lT = pnode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = pnode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = pnode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}

