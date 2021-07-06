#pragma once
#include"HModelManagerStructs.h"
//설계 개요
/*
1.FBX모델 데이터를 읽어 래스터라이스와 레이트레이싱에 필요한 리소스들을 생성하는 클래스
래스터라이즈의 경우 모든 모델들의 버텍스와 인덱스를 하나의 큰 버퍼에 저장하고 오프셋
으로 참조하는 방식으로 그려줄 것이기 때문에 개별 모델의 버퍼는 준비하지 않는다. 이 방식
을 통해서 모델마다 버텍스와 인덱스 버퍼를 바꿔줄 필요 없이 한번의 SetVertex, SetIndex
를 통해서 그려줄 수 있다. 또한 인스턴싱 드로우 방식을 취할 것 이기 때문에 인스턴스 정보
역시 이 클래스를 통해서 관리한다.
다만 레이트레이싱의 BLAS를 만들 때 개별 모델의 버텍스 인덱스 버퍼가 필요하기 때문에 매
모델을 로드할 때 해당 버퍼를 만들어준다. 하지만 위에서 언급했듯이 래스터라이즈 드로우는
모든 모델의 버텍스, 인덱스를 가진 큰 버퍼를 통해서 그려줄 것이기 때문에 굳이 개별 모델 
버퍼를 만들 때에서는 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_
STATE_INDEX_BUFFER 플래그가 필요 없다. 따라서 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_
RESOURCE 만 설정해준다.
모든 모델 데이터를 가지고 있는 큰 버퍼의 경우 매 프레임 추가 모델이 로드돼었는지 확인해서
(m_bModelDirty를 통해) 추가돼었을 경우 모든 버퍼를 합쳐서 만들어 준다.
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


	//void UpdateperInstanceBoneAnimationResouce_rasterize(HModel* pHModel, unsigned char shaderFlag);//모델단위로

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