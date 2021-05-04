#pragma once
#include"fbxsdk.h"
#include <vector>


#include"HSelfFormatDefines.h"

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
	unsigned int index;
	bool isDepthZero = false;
	std::vector<unsigned int> childIndex;
};


struct MeshData : public NodeData
{
	XMFLOAT4X4			   staticTM;	//�ִϸ��̼� ���� ��
	XMFLOAT4X4			   globalBindTM;//skin �ִϸ��̼� �� ��(bone�ϰ� ����� ������ TM)
	XMFLOAT4X4			   geometryTM;//�ǹ�����
	bool hasDeformingBone = false;
	//�� �޽��� �����ϴ� ���ؽ���. �ǹ��� ������ ���� ����. 
	std::vector<HModel_Vertex> vertices;


	//Generate�� �� �־��� ������
	unsigned int vertexOffset;
	unsigned int indexOffset;
	unsigned int indexCount;
	unsigned int vertexCount;

	BoundingBox boundingBox;
};

struct BoneData : public NodeData
{
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

	unsigned int currBoneIndex = 0;
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
	std::string m_fileName;
public:

	//mesh data
	std::vector<MeshData*> m_meshData_parents;
	std::vector<MeshData*> m_meshData;
	unsigned int		   m_currMeshIndex = 0;

	//bone data
	std::vector<BoneData*>		m_boneData_parents;				//�θ� ����(�ֻ����) ��
	std::vector<BoneData*>		m_boneData;						//��� ��
	unsigned int				m_currBoneIndex = 0;
	
	//fbx Import �����
	//���뺯��

	XMFLOAT4X4					m_axisChange;					//�� ��ǥ�� -> ���� ��ǥ��
	BoundingBox					m_boundingBox;					//�浹�ڽ�

	std::vector<HModel_Vertex>	m_allVertices;					//��� ���ؽ�
	std::vector<std::uint32_t>	m_allIndices;					//��� �ε���


	//animation data
	AnimationData m_animData;						//�ִϸ��̼� ������

public:
	HFbxImporter();
	~HFbxImporter();
	bool ImportModel(const char* filename);
	bool ImportAnimation(const char* filename);
	void WriteHModelFile();
	void WriteHAnimFile();
private:
	
	void CalAxisMaxToDirectX(FbxAxisSystem maxAxisSystem);
	void ProcessScene(FbxScene* pScene);
	void ProcessBones(FbxNode* pFbxNode, BoneData* pNode);
	void ProcessNodesRecursively(FbxNode* pFbxNode, NodeData* pParent, FbxNodeAttribute::EType nodeType);

	void ProcessMesh(FbxNode* pNode, MeshData* pNodeOutput);
	void ReadPosition(FbxMesh* inMesh, int inCtrlPointIndex, HModel_Vertex& vertex);
	void ReadTangentBasis(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, HModel_Vertex& vertex);
	void ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, HModel_Vertex& vertex);
	void ReadBoneWeight(std::vector<BoneIndexWeight>& boneIndexWeight, int ctrlPoint, HModel_Vertex& vertex);
	void CreateBoneWeight(FbxMesh* inMesh, std::vector<BoneIndexWeight>& boneIndexWeight, MeshData* pNodeOutput);
	void ShrinkBoneNumberToFour(std::vector<BoneIndexWeight>& boneIndexWeight);
	void BubbleSort(std::vector<std::pair<unsigned int, float>>& boneIndexWeights);

	int FindBoneIndex(std::string boneName);

	//�ִϸ��̼� �� ������ ����
	void ProcessAnimations(FbxScene* pScene, std::string& animName);

	
	

	void ProcessNodesRecursively_anim(FbxAnimLayer* pAnimLayer, FbxNode* pFbxNode, NodeAnimData* pParent);
	void ProcessKeyFrames(FbxAnimLayer* pAnimLayer, FbxNode* pFbxNode, NodeAnimData* pNodeAnim);
	FbxAMatrix GetGeometryTransformation(FbxNode* pnode);

	//���� ������ ����
	void CreateFinalVertex(MeshData* pNode);
	void GenerateNodeData();
	void MergeVertexAndMakeIndex(MeshData* pNode);
	void MergeBoundingBox(BoundingBox& mergedBoundingBox , MeshData* pNode);


	//��Ÿ�Լ�
	FbxAMatrix GetGeometryTransformaton(FbxNode* pNode);

	

};