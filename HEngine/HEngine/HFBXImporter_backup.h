#pragma once
#include"HModelManagerStructs.h"

enum class eAnimType
{
	eLegacyAnimation,
	eSkinAnimation
};

struct BoneIndexWeight
{
	std::vector<std::pair<unsigned int, float>> boneIndexWeights;
};

struct NodeData
{
	std::string nodeName;
	std::string typeName;
	NodeData* pParentNode;
	std::vector<NodeData*> pChildNodes;
};


struct MeshData : public NodeData
{
	XMFLOAT4X4			   staticTM;	//애니메이션 없을 때
	XMFLOAT4X4			   globalBindTM;//skin 애니메이션 일 때(bone하고 연결된 순간의 TM)

	bool hasDeformingBone = false;
	//이 메쉬를 구성하는 버텍스들. 피벗이 원점일 때의 기준. 
	std::vector<HVertex> vertices;


	//Generate될 때 넣어줄 데이터
	unsigned int vertexOffset;
	unsigned int indexOffset;
	unsigned int indexCount;
	unsigned int vertexCount;

	BoundingBox boundingBox;
};

struct BoneData : public NodeData
{
	unsigned int boneIndex;
	XMFLOAT4X4 localTTM;
	XMFLOAT4X4 invGlobalTTM;

	XMFLOAT4X4 invLinkMat;
};

struct KeyFrameData
{
	float time;
	//local
	XMFLOAT3 scale;
	XMFLOAT4 quatRot;
	XMFLOAT3 trans;
	//global
	XMFLOAT4X4 globalTM;
};

struct NodeAnimData : public NodeData
{
	std::vector<KeyFrameData> keyFrames;
};

struct AnimationData
{
	std::string name;
	eAnimType animType;
	float framesPerSecond;
	int	  totalFrames;
	float startTime;
	float endTime;
	float totalTime;

	std::vector<NodeAnimData*> meshAnim;
	std::vector<NodeAnimData*> boneAnim;
	std::vector<NodeAnimData*> allBoneAnim;
};

class HFbxImporter
{
private:
	//fbx Import과정에 쓰이는 자원들
	FbxManager* m_pFbxManager;
	FbxImporter* m_pImporter;
	FbxIOSettings* m_pIOSettings;
	FbxAxisSystem m_axisSystem;

public:

	//mesh data
	std::vector<MeshData*> m_meshData;
	
	//fbx Import 결과물
	//공통변수

	XMFLOAT4X4					m_axisChange;					//툴 좌표계 -> 게임 좌표계
	BoundingBox					m_boundingBox;					//충돌박스

	std::vector<HVertex>		m_allVertices;					//모든 버텍스
	std::vector<std::uint32_t>	m_allIndices;					//모든 인덱스
	//bone data
	std::vector<BoneData*>		m_boneData_parents;				//부모가 없는(최상단의) 본
	std::vector<BoneData*>		m_boneData;						//모든 본
	unsigned int				m_totalBoneCount = 0;

	//animation data
	std::vector<AnimationData*> m_animData;						//애니메이션 데이터

public:
	HFbxImporter();
	~HFbxImporter();
	void ImportFbx(const char* filename);
	void ImportAnimation(const char* filename);
private:
	
	void CalAxisMaxToDirectX(FbxAxisSystem maxAxisSystem);
	void ProcessScene(FbxScene* pScene);
	void ProcessBones(FbxNode* pFbxNode, BoneData* pNode);
	void ProcessNodesRecursively(FbxNode* pFbxNode, NodeData* pParent, FbxNodeAttribute::EType nodeType);

	void ProcessMesh(FbxNode* pNode, MeshData* pNodeOutput);
	void ReadPosition(FbxMesh* inMesh, int inCtrlPointIndex, HVertex& vertex);
	void ReadTangentBasis(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, HVertex& vertex);
	void ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, HVertex& vertex);
	void ReadBoneWeight(std::vector<BoneIndexWeight>& boneIndexWeight, int ctrlPoint, HVertex& vertex);
	void CreateBoneWeight(FbxMesh* inMesh, std::vector<BoneIndexWeight>& boneIndexWeight, MeshData* pNodeOutput);
	void ShrinkBoneNumberToFour(std::vector<BoneIndexWeight>& boneIndexWeight);
	void BubbleSort(std::vector<std::pair<unsigned int, float>>& boneIndexWeights);

	int FindBoneIndex(std::string boneName);

	//애니메이션 별 데이터 생성
	void ProcessAnimations(FbxScene* pScene, std::string& animName);

	bool CheckIfAnimAndModelMatch(AnimationData* pAnimData);
	bool CheckByNameMatchRecursively(NodeData* pMeshOrBone, NodeData* pAnimNode);

	void ProcessNodesRecursively_anim(FbxAnimLayer* pAnimLayer, FbxNode* pFbxNode, NodeAnimData* pParent, AnimationData* pAnimData);
	void ProcessKeyFrames(FbxAnimLayer* pAnimLayer, FbxNode* pFbxNode, NodeAnimData* pNodeAnim, AnimationData* pAnimData);
	FbxAMatrix GetGeometryTransformation(FbxNode* pnode);

	//최종 데이터 생성
	void CreateFinalVertex(MeshData* pNode);
	void GenerateNodeData();
	void MergeVertexAndMakeIndex(MeshData* pNode);
	void MergeBoundingBox(BoundingBox& mergedBoundingBox , MeshData* pNode);
};