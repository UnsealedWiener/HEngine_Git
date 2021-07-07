#pragma once
#include"HModelManagerStructs.h"
//���� ����
/*
1.�𵨵��� ���ؽ� ���� ������, ���ҽ�ȭ �� ������ �׸��� �ν��Ͻ��� ���� ���� �ν��Ͻ�����
�����͸� ������ �����ϴ� Ŭ�����̴�.

2.FBX�� �����͸� �о� �����Ͷ��̽��� ����Ʈ���̽̿� �ʿ��� ���ҽ����� �����ϴ� Ŭ����
�����Ͷ������� ��� ��� �𵨵��� ���ؽ��� �ε����� �ϳ��� ū ���ۿ� �����ϰ� ������
���� �����ϴ� ������� �׷��� ���̱� ������ ���� ���� ���۴� �غ����� �ʴ´�. �� ���
�� ���ؼ� �𵨸��� ���ؽ��� �ε��� ���۸� �ٲ��� �ʿ� ���� �ѹ��� SetVertex, SetIndex
�� ���ؼ� �׷��� �� �ִ�. ���� �ν��Ͻ� ��ο� ����� ���� �� �̱� ������ �ν��Ͻ� ����
���� �� Ŭ������ ���ؼ� �����Ѵ�.
�ٸ� ����Ʈ���̽��� BLAS�� ���� �� ���� ���� ���ؽ� �ε��� ���۰� �ʿ��ϱ� ������ ��
���� �ε��� �� �ش� ���۸� ������ش�. ������ ������ ����ߵ��� �����Ͷ����� ��ο��
��� ���� ���ؽ�, �ε����� ���� ū ���۸� ���ؼ� �׷��� ���̱� ������ ���� ���� �� 
���۸� ���� ������ D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_
STATE_INDEX_BUFFER �÷��װ� �ʿ� ����. ���� D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_
RESOURCE �� �������ش�.
��� �� �����͸� ������ �ִ� ū ������ ��� �� ������ �߰� ���� �ε�ž����� Ȯ���ؼ�
(m_bModelDirty�� ����) �߰��ž��� ��� ��� ���۸� ���ļ� ����� �ش�.

*/

namespace DX
{
	class DeviceResources;
}

//class ModelFbx;
class HFbxImporter;
struct NodeData;
struct MeshData;
struct BoneData;
struct AnimationData;
struct HModelFormat;
struct HAnimFormat;

class HModelManager
{
private:
	//�̱����� ���� ������ ����
	HModelManager() {};

	//����� ����̽�
	const DeviceResources*								    m_pDeviceResources = nullptr;
	//Upload Heap�� ���ҽ����� ������ �� ���
	GraphicsMemory*											m_pGraphicMemory = nullptr;
	//Frustum �Ǵ� Box Culling�� ���� ��ο� �ݿ��� ���ܽ����� �ν��Ͻ��� �ĺ��ϴµ� ����� ī�޶� Ŭ����
	const Camera*										    m_pCamera = nullptr;

	//�� �����Ϳ� �ִϸ��̼� ������
	//fbx �Ǵ� custom format�� �о�� �����Ѵ�.
	//���� �� �Ҵ�Ǵ� �ּ� ���� Ű ������ ���
	std::unordered_map<void*, std::shared_ptr<HModel>>		m_models;
	std::unordered_map<void*, std::shared_ptr<HAnim>>		m_anims;
	
	//instance data//��ü �ν��Ͻ� ���ڿ� ī�޶� ���� ���� �ν��Ͻ� ����
	std::unordered_map<void*, std::weak_ptr<HInstance>>		m_instances;
	std::vector<HInstance*>									m_visibleInstances;
	unsigned int											m_totalInstance = 0;
	unsigned int											m_visibleInstance = 0;

	//��� �𵨵��� ���ؽ��� �ε����� ������ ������
	std::vector<HVertex>									m_allVertices;
	std::vector<std::uint32_t>								m_allIndices;
	ComPtr<ID3D12Resource>									m_allVerticesResource;
	ComPtr<ID3D12Resource>									m_allIndicesResource;
	UINT													m_allVerticesCount = 0;
	UINT													m_allIndicesCount = 0;

	//����Ʈ���̽� ���̴����� ���� TLAS�� structured buffer��
	//(raytracing Ư���� ��� �ν��Ͻ����� �������� �ѹ��� Set����� �ϱ� ������ Model�̳� Instance ���� �ƴ�
	//Manager �ܿ��� �� ���۸� ���� �ִ´�.)
	ComPtr<ID3D12Resource>									m_pTopLevelAccelerationStructures;
	ComPtr<ID3D12Resource>									m_pTopLevelAccelerationStructureScratches;
	SharedGraphicsResource									m_pRaytracingInstanceDescs;
	
	SharedGraphicsResource									m_raytracingStructuredBuffer_perInstance;
	SharedGraphicsResource									m_raytracingStructuredBuffer_perModel;

	//Bone�� �ִ� �𵨵��� ���� �ִϸ��̼� ����, �ð��� ���� �ٸ��� �ν��Ͻ��� ���ؽ���
	//Raytracing�� ���� ���ҽ�
	//*Raytracing�� Rasterize�� �ٸ��� ���̴��� �����ϱ� ���� ���� Geometry�� �Ѱ���� �Ѵ�.
	//*�� �ִϸ��̼ǵ� ���ο� ���ؽ��� ���۷� ���� BLAS ������Ʈ�� ����ؾ� �Ѵ�.
	ComPtr<ID3D12Resource>									m_pAllDynamicInstacedVertices;

	//�� ���ҽ��� �������ֱ� ���� Compute Shader �ڿ���
	ComPtr<ID3D12PipelineState>								m_pComputePSO;
	ComPtr<ID3D12RootSignature>								m_pComputeShaderRootSignature;


	//�� ��Ƽ �÷���, ���ο� ���� �ε� ���� �� true�� ��ȯ, ��� ���ؽ��� ��� �ִ� ���۸� �������� �� ���
	bool													m_bModelDirty = false;
	bool													m_bRaytracingDirty = false;

public:
	static HModelManager* GetInstance();
	void Initialize(DeviceResources* pDeviceResources, GraphicsMemory* pGraphicMemory, Camera* pCamera);

	unsigned int GetTotalInstanceCnt() const{ return m_instances.size(); }
	unsigned int GetVisibleInstanceCnt() const{ return m_visibleInstance; }
	
	//1. ���ο� ���� �ε�Ǹ� ��ü ���ؽ� ���۸� ���� 
	//2. �𵨵��� �ν��Ͻ� ����ִ��� Ȯ�� 
	//3. ī�޶��� �� ��������(�Ǵ� �ڽ�)�� �ν��Ͻ��� ����ִ��� Ȯ���ϰ� �����ִ� Instance���� ����� ����. 
	void Update();

	//1. Rasterize�� Instancing Draw�� ���� PSO��, �𵨺�, �ν��Ͻ��� Structured Buffer�� �������ش�.
	void UpdateResource_raterize();
	
	//1. BLAS�� ������Ʈ�ϱ� ���� ��� Dynamic Instanced vertex���� ���� �� �ִ� ���۸� ����
	//2. �� ���۸� Compuste Shader���� ������Ʈ �� �� ����� Structured Buffer(�𵨺� �� ����, ���ؽ� ���� ��)�� ����
	//3. Raytracing pipe line���� ����� Structured Buffer�� ����
	void UpdateResouce_raytracing();

	//1. Rasterize�� Raytracing���� �������� ���� �ִϸ��̼ǵ� Bone�������� �ִϸ��̼� ����, �ð��� ���� ������Ʈ
	void UpdateAllInstancedBoneAnimResource_common();

	//m_allVerticesResource, m_allIndicesResource�� ����ϱ� ���� View��
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()const;
	D3D12_INDEX_BUFFER_VIEW	 GetIndexBufferView()const;

	//fbx �Ǵ� ��ü ������ �о�鿩 �� �Ǵ� �ִϸ��̼� �����͸� �������ִ� �Լ��� 
	HModelData* CreateModelFromFbxFile(std::string fbxName, ResourceUploadBatch& resourceBatch);
	HAnimData* CreateAnimationFromFbxFiles(std::vector<std::string> fbxNames);
	HModelData* CreateModelFromHModelFile(std::string fbxName, ResourceUploadBatch& resourceBatch);
	HAnimData* CreateAnimationFromHAnimFiles(std::vector<std::string> fbxNames);

private:
	//Import �� fbx�� ���� �� �����͸� ����
	void CreateModelFromLoadedHModelFile(HModelFormat* pHmodel, HModel* pModel, ResourceUploadBatch& resourceBatch);
	//Import �� hmodel(��ü����) ������ ���� �ִϸ��̼� ������ ����
	void CreateModelFromImportedFbx(HFbxImporter* pFbxImporter, ResourceUploadBatch& resourceBatch, HModel* pModel);
	void CreateAllMeshData(MeshData* pFbxMeshNode, HMesh* pHNode);
	void LinkMeshChild(MeshData* pFbxMeshNode, HMesh* pHNode, HModel* pModel);
	void CreateAllBoneData(BoneData* pFbxBoneNode, HBone* pHBone);
	void LinkBoneChild(BoneData* pFbxBoneNode, HBone* pHBone, HModel* pModel);
	void PickZeroDepthMeshBone(HFbxImporter* pFbxImporter, HModel* pModel);
	void CreateRaytracingResource(HModel* pModel, ResourceUploadBatch& resourceBatch);
	void CreateRaytracingGeometryDesc(HModel* pModel, D3D12_RAYTRACING_GEOMETRY_DESC& rtDescs);

	//Import �� fbx�� ���� �ִϸ��̼� �����͸� ����
	void CreateAnimationFromImportedFbx(HFbxImporter* pFbxImporter, HAnim* pAnim);
	void CreateAnimDataFromImportedFbx(HBoneAnims* pHAnim, HFbxImporter* pFbxImporter);
	//Import �� hanim(��ü����) ������ ���� �ִϸ��̼� ������ ����
	void CreateAnimDataFromLoadedHAnimFile(HBoneAnims* pHAnim, HAnimFormat* pHAnimFormat);

	//�� �Ŵ��� ������Ʈ �Լ���
	void CheckInstanceAlive();
	void MergeVertexIndexData();
	void CheckFrustumCulling();
	void CalNodeVertexIndexOffsetRecursively(HMesh* pNode, UINT ModelVertexOffset, UINT ModelIndexOffset);
	
	//�� �ִϸ��̼� ��Ʈ���� ��� �Լ���(�����Ͷ�����, ����Ʈ���̽� ����)
	void UpdateInstanceBoneAnimResource_common(std::vector<Matrix>& allBoneTM, HInstance* pHinstance, HModel* pHModel);
	void UpdateBoneAnimRecusively_local_common(std::vector<Matrix>& allBoneData, std::unordered_map<std::string, Matrix>& instanceBoneTM,
		HBone* pBone, HBoneAnims* pAnim, HBoneAnims* pSecondAnim, float animTime, float animTimeSecond,float lerpValue, Matrix parentTM);
	void CalculateBoneAnim(Matrix* boneLocalTM, HBoneAnim const* pBoneAnim, float time, Vector3& outScale, Quaternion& outQuat, Vector3& outTrans);

	//�����Ͷ����� ������Ʈ �Լ���
	void UpdatePerNodeResouceRecursively_rasterize(HMesh* pNode, UINT modelBoneCount);
	void UpdatePerModelResouce_rasterize(HModel* pHModel);
	void UpdatePerInstanceResouce_rasterize(HModel* pHModel, PSOTYPE psoType);
	void UpdatePerPSOResource_rasterize(HModel* pHModel, PSOTYPE psoType, UINT& instanceOffset);
	void UpdatePerInstanceStructuredBuffer_onlyVisible_rasterize(/*std::unordered_map<void*, std::shared_ptr<HInstance>> const& instances*/
		std::vector<HInstance*>& instances,
		SharedGraphicsResource* pStructuredBuffer);

	//����Ʈ���̽� ������Ʈ �Լ���
	void UpdatePerModelResource_raytracing();
	void UpdatePerInstanceResource_raytracing();
	void UpdatePerNodeResource_raytracing();
	void UpdatePerNodeResourceRecusively_raytracing(HMesh* pNode,Matrix AxisChange, std::vector<HSturecturedBuffer_Raytracing_PerNode>& perNode);
	void UpdateRaytracingBottomLevelAccelerationStructure_raytracing(ID3D12GraphicsCommandList4* cmdList);
	void CreateBLAS(ID3D12GraphicsCommandList4* cmdList, HInstance* pHInstance, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc);
	void UpdateBLAS(ID3D12GraphicsCommandList4* cmdList, HInstance* pHInstance, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc);
	void UpdateRaytracingTopLevelAccelerationStructure_raytracing(ID3D12GraphicsCommandList4* cmdList);

	//m_pAllDynamicInstacedVertices�� �������ֱ� ���� Compute Shader �ڿ��� ����
	//Initialize �ܰ迡�� ��
	void CreateComputePSO_lightCalculation();
	void CreateComputeRootSignature();

	//m_pAllDynamicInstacedVertices�� �������ֱ� ���� Compute Shader ������Ʈ �� ���� �Լ���
	//�� ������ ��
	void UpdateAllInstancedVertices_raytracing();
	unsigned int GetTotalDynamicVertexCount();
	void CreateVertexBuffer_raytracing(unsigned int vertexCount);
	void CreatePerDynamicModelDataForComputeShader();
	void UpdateVertexBuffer_raytracing(ID3D12GraphicsCommandList* cmdList);

	friend class HRasterize;
	friend class HRaytracing;
	friend class HPicking;
	friend struct HModel;
};