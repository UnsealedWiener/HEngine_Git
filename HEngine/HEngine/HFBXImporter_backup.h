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
	XMFLOAT4X4			   staticTM;	//�ִϸ��̼� ���� ��
	XMFLOAT4X4			   globalBindTM;//skin �ִϸ��̼� �� ��(bone�ϰ� ����� ������ TM)

	bool hasDeformingBone = false;
	//�� �޽��� �����ϴ� ���ؽ���. �ǹ��� ������ ���� ����. 
	std::vector<HVertex> vertices;


	//Generate�� �� �־��� ������
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
	//fbx Import������ ���̴� �ڿ���
	FbxManager* m_pFbxManager;
	FbxImporter* m_pImporter;
	FbxIOSettings* m_pIOSettings;
	FbxAxisSystem m_axisSystem;

public:

	//mesh data
	std::vector<MeshData*> m_meshData;
	
	//fbx Import �����
	//���뺯��

	XMFLOAT4X4					m_axisChange;					//�� ��ǥ�� -> ���� ��ǥ��
	BoundingBox					m_boundingBox;					//�浹�ڽ�

	std::vector<HVertex>		m_allVertices;					//��� ���ؽ�
	std::vector<std::uint32_t>	m_allIndices;					//��� �ε���
	//bone data
	std::vector<BoneData*>		m_boneData_parents;				//�θ� ����(�ֻ����) ��
	std::vector<BoneData*>		m_boneData;						//��� ��
	unsigned int				m_totalBoneCount = 0;

	//animation data
	std::vector<AnimationData*> m_animData;						//�ִϸ��̼� ������

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

	//�ִϸ��̼� �� ������ ����
	void ProcessAnimations(FbxScene* pScene, std::string& animName);

	bool CheckIfAnimAndModelMatch(AnimationData* pAnimData);
	bool CheckByNameMatchRecursively(NodeData* pMeshOrBone, NodeData* pAnimNode);

	void ProcessNodesRecursively_anim(FbxAnimLayer* pAnimLayer, FbxNode* pFbxNode, NodeAnimData* pParent, AnimationData* pAnimData);
	void ProcessKeyFrames(FbxAnimLayer* pAnimLayer, FbxNode* pFbxNode, NodeAnimData* pNodeAnim, AnimationData* pAnimData);
	FbxAMatrix GetGeometryTransformation(FbxNode* pnode);

	//���� ������ ����
	void CreateFinalVertex(MeshData* pNode);
	void GenerateNodeData();
	void MergeVertexAndMakeIndex(MeshData* pNode);
	void MergeBoundingBox(BoundingBox& mergedBoundingBox , MeshData* pNode);
};