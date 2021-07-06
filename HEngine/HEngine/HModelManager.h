#pragma once
#include"HModelManagerStructs.h"
//���� ����
/*
1.FBX�� �����͸� �о� �����Ͷ��̽��� ����Ʈ���̽̿� �ʿ��� ���ҽ����� �����ϴ� Ŭ����
�����Ͷ������� ��� ��� �𵨵��� ���ؽ��� �ε����� �ϳ��� ū ���ۿ� �����ϰ� ������
���� �����ϴ� ������� �׷��� ���̱� ������ ���� ���� ���۴� �غ����� �ʴ´�. �� ���
�� ���ؼ� �𵨸��� ���ؽ��� �ε��� ���۸� �ٲ��� �ʿ� ���� �ѹ��� SetVertex, SetIndex
�� ���ؼ� �׷��� �� �ִ�. ���� �ν��Ͻ� ��ο� ����� ���� �� �̱� ������ �ν��Ͻ� ����
���� �� Ŭ������ ���ؼ� �����Ѵ�.
�ٸ� ����Ʈ���̽��� BLAS�� ���� �� ���� ���� ���ؽ� �ε��� ���۰� �ʿ��ϱ� ������ ��
���� �ε��� �� �ش� ���۸� ������ش�. ������ ������ ����ߵ��� �����Ͷ����� ��ο��
��� ���� ���ؽ�, �ε����� ���� ū ���۸� ���ؼ� �׷��� ���̱� ������ ���� ���� �� 
���۸� ���� �������� D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_
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
	HModelManager() {};
	const DeviceResources* m_pDeviceResources;
	GraphicsMemory* m_pGraphicMemory;
	const Camera* m_pCamera;

	//status
	unsigned int m_visibleInstance = 0;
	unsigned int m_totalInstance = 0;

	//model data
	std::unordered_map<void*, std::shared_ptr<HModel>>	  m_models;
	std::unordered_map<void*, std::shared_ptr<HAnim>>	  m_anims;
	
	//instance data
	std::unordered_map<void*, std::weak_ptr<HInstance>> m_instances;
	std::vector<HInstance*> m_visibleInstances;


	std::vector<HVertex>												  m_allVertices;
	std::vector<std::uint32_t>											  m_allIndices;
	ComPtr<ID3D12Resource>												  m_allVerticesResource;
	ComPtr<ID3D12Resource>												  m_allIndicesResource;
	UINT																  m_allVerticesCount;
	UINT																  m_allIndicesCount;

	//raytracing
	SharedGraphicsResource												  m_raytracingStructuredBuffer_perInstance;
	SharedGraphicsResource												  m_raytracingStructuredBuffer_perModel;

	ComPtr<ID3D12Resource>									  m_pTopLevelAccelerationStructures;
	ComPtr<ID3D12Resource>									  m_pTopLevelAccelerationStructureScratches;
	
	SharedGraphicsResource								  m_pRaytracingInstanceDescs;

	//working

	ComPtr<ID3D12Resource>												  m_pAllInstacedVertices;

	//Compute Shader Resources
	ComPtr<ID3D12PipelineState>											  m_pComputePSO;
	ComPtr<ID3D12RootSignature>											  m_pComputeShaderRootSignature;


	bool m_bModelDirty = false;
	bool m_bRaytracingDirty = false;
	//used in raytracing
	bool m_bInstanceDirty = false;
public:
	static HModelManager* GetInstance();
	void Initialize(DeviceResources* pDeviceResources, GraphicsMemory* pGraphicMemory, Camera* pCamera);

	/*MODELHANDLE LoadModel(std::vector<std::string> fbxFiles, ResourceUploadBatch& resourceBatch);
	INSTANCEHANDLE AddInstance(MODELHANDLE hModel, unsigned char shaderFlag,
		Matrix worldTM, std::shared_ptr<UINT> matIndex0, std::shared_ptr<UINT> matIndex1);

	void DeleteModel(MODELHANDLE hModel);
	void DeleteInstance(INSTANCEHANDLE hInstance);

	void SetInstanceWorldTM(INSTANCEHANDLE hInstance, Matrix worldTM);
	void SetInstanceMat(INSTANCEHANDLE hInstance, std::shared_ptr<UINT> matIndex, unsigned int slot = 0);
	void SetInstanceAnim(INSTANCEHANDLE hInstance, std::string animation);
	void SetInstanceAnimTime(INSTANCEHANDLE hInstance, float animationTime);
	void SetInstanceAnimTimeByRatio(INSTANCEHANDLE hInstance, float ratio);
	void SetInstanceMatLerpValue_twoMat(INSTANCEHANDLE hInstance, float materialLerpValue);*/

	unsigned int GetTotalInstanceCnt() const{ return m_instances.size(); }
	unsigned int GetVisibleInstanceCnt() const{ return m_visibleInstance; }

	void Update();
	void UpdateResource_raterize();
	void UpdateResouce_raytracing();
	void UpdateAllInstancedBoneAnimResource_common();
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()const;
	D3D12_INDEX_BUFFER_VIEW	 GetIndexBufferView()const;

	//HModelRawData* GetModelRawData(MODELHANDLE hModel) { return &m_models[hModel].get()->rawData; }

	HModelData* CreateModelFromFbxFile(std::string fbxName, ResourceUploadBatch& resourceBatch);
	HAnimData* CreateAnimationFromFbxFiles(std::vector<std::string> fbxNames);
	HModelData* CreateModelFromHModelFile(std::string fbxName, ResourceUploadBatch& resourceBatch);
	HAnimData* CreateAnimationFromHAnimFiles(std::vector<std::string> fbxNames);

private:
	//anim loading
	void CreateAnimationFromImportedFbx(HFbxImporter* pFbxImporter, HAnim* pAnim);

	//model loading
	void CreateModelFromLoadedHModelFile(HModelFormat* pHmodel, HModel* pModel, ResourceUploadBatch& resourceBatch);
	void CreateModelFromImportedFbx(HFbxImporter* pFbxImporter, ResourceUploadBatch& resourceBatch, HModel* pModel);
	void CreateAllMeshData(MeshData* pFbxMeshNode, HMesh* pHNode);
	void LinkMeshChild(MeshData* pFbxMeshNode, HMesh* pHNode, HModel* pModel);
	void CreateAllBoneData(BoneData* pFbxBoneNode, HBone* pHBone);
	void LinkBoneChild(BoneData* pFbxBoneNode, HBone* pHBone, HModel* pModel);
	void PickZeroDepthMeshBone(HFbxImporter* pFbxImporter, HModel* pModel);
	void CreateRaytracingResource(HModel* pModel, ResourceUploadBatch& resourceBatch);

	void CreateAnimDataFromImportedFbx(HBoneAnims* pHAnim, HFbxImporter* pFbxImporter);
	void CreateAnimDataFromLoadedHAnimFile(HBoneAnims* pHAnim, HAnimFormat* pHAnimFormat);
	//void CreateNodesTransformMatrixBufferRecursively(HMesh* pNode, Matrix AxisChange, std::vector<XMFLOAT3X4A>& transformMats);
	//void CreateRaytracingGeometryDescRecursively(HMesh* pNode, HModel* pModel, UINT& transformBufferIndex, std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& rtDescs);
	void CreateRaytracingGeometryDesc(HModel* pModel, D3D12_RAYTRACING_GEOMETRY_DESC& rtDescs);

	//model manager update
	void CheckInstanceAlive();
	void MergeVertexIndexData();

	void CheckFrustumCulling();
	void CalNodeVertexIndexOffsetRecursively(HMesh* pNode, UINT ModelVertexOffset, UINT ModelIndexOffset);
	
	//rasterize resource update
	void UpdatePerNodeResouceRecursively_rasterize(HMesh* pNode, UINT modelBoneCount);
	void UpdatePerModelResouce_rasterize(HModel* pHModel);
	void UpdatePerInstanceResouce_rasterize(HModel* pHModel, PSOTYPE psoType);
	void UpdatePerPSOResource_rasterize(HModel* pHModel, PSOTYPE psoType, UINT& instanceOffset);
	void UpdatePerInstanceStructuredBuffer_onlyVisible_rasterize(/*std::unordered_map<void*, std::shared_ptr<HInstance>> const& instances*/
		std::vector<HInstance*>& instances,
		SharedGraphicsResource* pStructuredBuffer);


	//void UpdateperInstanceBoneAnimationResouce_rasterize(HModel* pHModel, unsigned char shaderFlag);//�𵨴�����

	void UpdateInstanceBoneAnimResource_common(std::vector<Matrix>& allBoneTM, HInstance* pHinstance, HModel* pHModel);
	//void UpdateBoneAnimRecusively_local_common(std::vector<Matrix>& boneData, float animTime, HBone* pBone, HAnim* pAnim, Matrix parentTM);
	void UpdateBoneAnimRecusively_local_common(std::vector<Matrix>& allBoneData, std::unordered_map<std::string, Matrix>& instanceBoneTM,
		HBone* pBone, HBoneAnims* pAnim, HBoneAnims* pSecondAnim, float animTime, float animTimeSecond,float lerpValue, Matrix parentTM);
	void CalculateBoneAnim(Matrix* boneLocalTM, HBoneAnim const* pBoneAnim, float time, Vector3& outScale, Quaternion& outQuat, Vector3& outTrans);

	//void UpdateBoneAnim_global(std::vector<Matrix>& boneData, float animTime, HBone* pBone, HBoneAnims* pAnim);

	//raytracing resource update
	void UpdatePerModelResource_raytracing();
	void UpdatePerInstanceResource_raytracing();
	void UpdatePerNodeResource_raytracing();
	void UpdatePerNodeResourceRecusively_raytracing(HMesh* pNode,Matrix AxisChange, std::vector<HSturecturedBuffer_Raytracing_PerNode>& perNode);
	
	void UpdateRaytracingBottomLevelAccelerationStructure_raytracing(ID3D12GraphicsCommandList4* cmdList);
	void CreateBLAS(ID3D12GraphicsCommandList4* cmdList, HInstance* pHInstance, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc);
	void UpdateBLAS(ID3D12GraphicsCommandList4* cmdList, HInstance* pHInstance, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc);

	void UpdateRaytracingTopLevelAccelerationStructure_raytracing(ID3D12GraphicsCommandList4* cmdList);

	//working
	void CreateComputePSO_lightCalculation();
	void CreateComputeRootSignature();

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