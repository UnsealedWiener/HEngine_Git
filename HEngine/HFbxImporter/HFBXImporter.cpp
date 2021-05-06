
#include"HFbxUtil.h"
#include"HFBXImporter.h"
#include<iostream>
#include<fstream>


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

	for (int i = 0; i < m_meshData.size(); i++)
	{
		delete m_meshData[i];
	}

	for (int i = 0; i < m_boneData.size(); i++)
	{
		delete m_boneData[i];
	}

	for (int i = 0; i < m_animData.allBoneAnim.size(); i++)
	{
		delete m_animData.allBoneAnim[i];
	}
}

bool HFbxImporter::ImportModel(const char* filename)
{
	// Use the first argument as the filename for the importer.
	if (!m_pImporter->Initialize(filename, -1, m_pFbxManager->GetIOSettings()))
	{
		std::cout << "The model fbx file is not found. Or the file path is not english." << "\n";
		return false;
	}

	std::cout << "fbx file load succees." << "\n";

	std::string fullPath = filename;

	std::size_t dot = fullPath.find_last_of(".");
	std::size_t slash = fullPath.find_last_of("/");

	m_fileName = fullPath.substr(slash + 1, dot - slash - 1);

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

	return true;

}

bool HFbxImporter::ImportAnimation(const char* filename)
{
	if (!m_pImporter->Initialize(filename, -1, m_pFbxManager->GetIOSettings()))
	{
		std::cout << "The model fbx file is not found. Or the file path is not english." << "\n";
		return false;
	}

	std::cout << "fbx file load succees." << "\n";

	std::string fullPath = filename;

	std::size_t dot = fullPath.find_last_of(".");
	std::size_t slash = fullPath.find_last_of("/");

	m_fileName = fullPath.substr(slash + 1, dot - slash - 1);

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
	std::string animName = fileName.substr(fileName.find_last_of("@")+ 1 , fileName.find_last_of(".FBX") - fileName.find_last_of("@") - 4);

	ProcessAnimations(pScene, animName);

	pScene->Destroy();

	return true;
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
				pNode->index = m_currBoneIndex++;

				ProcessBones(pFbxNode, (BoneData*)pNode);
				m_boneData.push_back((BoneData*)pNode);
				if (pParent == nullptr)
					m_boneData_parents.push_back((BoneData*)pNode);
				break;
			case fbxsdk::FbxNodeAttribute::eMesh:
				pNode = new MeshData;
				pNode->index = m_currMeshIndex++;
				ProcessMesh(pFbxNode, (MeshData*)pNode);
				m_meshData.push_back((MeshData*)pNode);
				if (pParent == nullptr)
				{
					m_meshData_parents.push_back((MeshData*)pNode);
				}
				break;

			default:
				break;
			}

			pNode->nodeName = pFbxNode->GetName();
			pNode->typeName = GetAttributeTypeName(nodeType);


			if (pParent != nullptr)
			{
				pParent->childIndex.push_back(pNode->index);
			}
			else
			{
				pNode->isDepthZero = true;
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
	std::vector<HModel_Vertex> vertices;
	vertices.resize(triangleCount * 3);

	std::vector<BoneIndexWeight> boneIndexWeight(controlPointCount);

	CreateBoneWeight(pMesh, boneIndexWeight, pNodeOutput);
	ShrinkBoneNumberToFour(boneIndexWeight);

	for (unsigned int i = 0; i < triangleCount; i++)
	{
		for (int j = 0; j < polygonSize; j++, vertexCounter++)
		{
			HModel_Vertex vertex = {};

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

	pNodeOutput->geometryTM = ConvertMatrix(GetGeometryTransformation(pNode));
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

void HFbxImporter::ReadPosition(FbxMesh* inMesh, int inCtrlPointIndex, HModel_Vertex& vertex)
{
	vertex.pos = ConvertFloat3(inMesh->GetControlPointAt(inCtrlPointIndex));
}

void HFbxImporter::ReadTangentBasis(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, HModel_Vertex& vertex)
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

void HFbxImporter::ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, HModel_Vertex& vertex)
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

void HFbxImporter::ReadBoneWeight(std::vector<BoneIndexWeight>& boneIndexWeight, int ctrlPoint, HModel_Vertex& vertex)
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

void HFbxImporter::BubbleSort(std::vector<std::pair<unsigned int, float>>& boneIndexWeights)
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
			boneIndex = Bone->index;
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

	m_animData.framesPerSecond = framesPerSecond;
	m_animData.totalFrames = (int)((endTime - startTime) * (double)framesPerSecond);
	m_animData.startTime = startTime;
	m_animData.endTime = endTime;
	m_animData.totalTime = endTime - startTime;
	m_animData.name = animName.c_str();

	m_animData.allBoneAnim.reserve(m_boneData.size());

	FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>();

	ProcessNodesRecursively_anim(pAnimLayer, pScene->GetRootNode(), nullptr);

	//if (CheckIfAnimAndModelMatch(pAnimData) != true)
	//{
	//	HerrorMessage(L"model fbx and anim fbx mismatch")
	//		throw std::exception("The animation data doesn't match model data.");
	//}



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

void HFbxImporter::ProcessNodesRecursively_anim(FbxAnimLayer* pAnimLayer, FbxNode* pFbxNode, NodeAnimData* pParent)
{
	NodeAnimData* pNode = nullptr;

	FbxNodeAttribute* attribute = pFbxNode->GetNodeAttribute();

	if (attribute != nullptr)
	{

		FbxNodeAttribute::EType nodeType = attribute->GetAttributeType();

		if (nodeType == fbxsdk::FbxNodeAttribute::eSkeleton)
		{
			pNode = new NodeAnimData;
			pNode->nodeName = pFbxNode->GetName();
			pNode->typeName = GetAttributeTypeName(nodeType);
			pNode->index = m_animData.currBoneIndex++;

			ProcessKeyFrames(pAnimLayer, pFbxNode, pNode);
			m_animData.allBoneAnim.push_back(pNode);
			if (pParent == nullptr)
				m_animData.boneAnim.push_back(pNode);

			if (pParent != nullptr)
			{
				pParent->childIndex.push_back(pNode->index);
			}
		}
	}

	for (int i = 0; i < pFbxNode->GetChildCount(); i++)
	{
		ProcessNodesRecursively_anim(pAnimLayer, pFbxNode->GetChild(i), pNode);
	}
}

void HFbxImporter::ProcessKeyFrames(FbxAnimLayer* pAnimLayer, FbxNode* pFbxNode, NodeAnimData* pNodeAnim)
{
	double startTime = m_animData.startTime;

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

	Matrix finalTM = pNode->geometryTM;
	finalTM *= nodeToScene;

	for (int i = 0; i < pNode->vertices.size(); i++)
	{
		pNode->vertices[i].pos = Vector3::Transform(pNode->vertices[i].pos, finalTM);
		pNode->vertices[i].normal = Vector3::TransformNormal(pNode->vertices[i].normal, finalTM);
		pNode->vertices[i].tangentU = Vector3::TransformNormal(pNode->vertices[i].tangentU, finalTM);
		pNode->vertices[i].biNormV = Vector3::TransformNormal(pNode->vertices[i].biNormV, finalTM);
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

}

void HFbxImporter::MergeBoundingBox(BoundingBox& mergedBoundingBox, MeshData* pNode)
{
	BoundingBox::CreateMerged(mergedBoundingBox, mergedBoundingBox, pNode->boundingBox);
}

FbxAMatrix HFbxImporter::GetGeometryTransformaton(FbxNode* pNode)
{
	if (!pNode)
	{
		throw;
	}

	const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}

void HFbxImporter::WriteHModelFile()
{
	std::string outputFile = m_fileName;
	outputFile += ".hmodel";

	std::ofstream writeFile(outputFile.c_str(), std::ostream::binary);

	HModel_Header header;
	header.boneCount = m_boneData.size();
	header.meshCount = m_meshData.size();
	header.totalIndexCount = m_allIndices.size();
	header.totalVertexCount = m_allVertices.size();

	writeFile.write(reinterpret_cast<char*>(&header), sizeof(HModel_Header));

	HModel_CommonValue commonValue;
	commonValue.axisSystemModify = m_axisChange;
	commonValue.boundingBox = m_boundingBox;

	writeFile.write(reinterpret_cast<char*>(&commonValue), sizeof(HModel_CommonValue));

	for (int i = 0; i < m_meshData.size(); i++)
	{
		writeFile.write(m_meshData[i]->nodeName.c_str(), sizeof(char) * 256);
		writeFile.write(reinterpret_cast<char*>(&m_meshData[i]->isDepthZero), sizeof(bool));
		writeFile.write(reinterpret_cast<char*>(&m_meshData[i]->index), sizeof(unsigned int));

		unsigned int childCount = m_meshData[i]->childIndex.size();
		writeFile.write(reinterpret_cast<char*>(&childCount), sizeof(unsigned int));

		for (int j = 0; j < childCount; j++)
		{
			writeFile.write(reinterpret_cast<char*>(&m_meshData[i]->childIndex[j]), sizeof(unsigned int));
		}
	}

	for (int i = 0; i < m_boneData.size(); i++)
	{
		writeFile.write(m_boneData[i]->nodeName.c_str(), sizeof(char) * 256);
		writeFile.write(reinterpret_cast<char*>(&m_boneData[i]->isDepthZero), sizeof(bool));
		writeFile.write(reinterpret_cast<char*>(&m_boneData[i]->index), sizeof(unsigned int));

		unsigned int childCount = m_boneData[i]->childIndex.size();
		writeFile.write(reinterpret_cast<char*>(&childCount), sizeof(unsigned int));

		for (int j = 0; j < childCount; j++)
		{
			writeFile.write(reinterpret_cast<char*>(&m_boneData[i]->childIndex[j]), sizeof(unsigned int));
		}

		writeFile.write(reinterpret_cast<char*>(&m_boneData[i]->invGlobalTTM), sizeof(XMFLOAT4X4));
	}

	writeFile.write(reinterpret_cast<char*>(m_allVertices.data()), m_allVertices.size() * sizeof(HModel_Vertex));
	writeFile.write(reinterpret_cast<char*>(m_allIndices.data()), m_allIndices.size() * sizeof(std::uint32_t));

	writeFile.close();

	std::cout << "Creating hmodel file success." << "\n";
}

void HFbxImporter::WriteHAnimFile()
{
	std::string outputFile = m_fileName;
	outputFile += ".hanim";

	std::ofstream writeFile(outputFile.c_str(), std::ostream::binary);

	writeFile.write(m_animData.name.c_str(), sizeof(char) * 256);
	writeFile.write(reinterpret_cast<char*>(&m_animData.totalTime), sizeof(float));

	unsigned int allBoneCount = m_animData.allBoneAnim.size();

	writeFile.write(reinterpret_cast<char*>(&allBoneCount), sizeof(unsigned int));

	for (int i = 0; i < allBoneCount; i++)
	{
		writeFile.write(m_animData.allBoneAnim[i]->nodeName.c_str(), sizeof(char) * 256);
		writeFile.write(reinterpret_cast<char*>(&m_animData.allBoneAnim[i]->index), sizeof(unsigned int));

		unsigned int keyCount = m_animData.allBoneAnim[i]->keyFrames.size();

		writeFile.write(reinterpret_cast<char*>(&keyCount), sizeof(unsigned int));

		writeFile.write(reinterpret_cast<char*>(m_animData.allBoneAnim[i]->keyFrames.data()), sizeof(HAnim_Bone_KeyFrame) * keyCount);
	}

	writeFile.close();

	std::cout << "Creating hmodel file success." << "\n";
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

