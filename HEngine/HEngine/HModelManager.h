#pragma once
#include"HModelManagerStructs.h"
//설계 개요
/*
1.모델들의 버텍스 원본 데이터, 리소스화 된 데이터 그리고 인스턴싱을 위한 개별 인스턴스들의
데이터를 가지고 관리하는 클래스이다.

2.FBX모델 데이터를 읽어 래스터라이스와 레이트레이싱에 필요한 리소스들을 생성하는 클래스
래스터라이즈의 경우 모든 모델들의 버텍스와 인덱스를 하나의 큰 버퍼에 저장하고 오프셋
으로 참조하는 방식으로 그려줄 것이기 때문에 개별 모델의 버퍼는 준비하지 않는다. 이 방식
을 통해서 모델마다 버텍스와 인덱스 버퍼를 바꿔줄 필요 없이 한번의 SetVertex, SetIndex
를 통해서 그려줄 수 있다. 또한 인스턴싱 드로우 방식을 취할 것 이기 때문에 인스턴스 정보
역시 이 클래스를 통해서 관리한다.
다만 레이트레이싱의 BLAS를 만들 때 개별 모델의 버텍스 인덱스 버퍼가 필요하기 때문에 매
모델을 로드할 때 해당 버퍼를 만들어준다. 하지만 위에서 언급했듯이 래스터라이즈 드로우는
모든 모델의 버텍스, 인덱스를 가진 큰 버퍼를 통해서 그려줄 것이기 때문에 굳이 개별 모델 
버퍼를 만들 때에는 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_
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
	//싱글턴을 위한 생성자 제한
	HModelManager() {};

	//어댑터 디바이스
	const DeviceResources*								    m_pDeviceResources = nullptr;
	//Upload Heap의 리소스들을 생성할 때 사용
	GraphicsMemory*											m_pGraphicMemory = nullptr;
	//Frustum 또는 Box Culling을 통해 드로우 콜에서 제외시켜줄 인스턴스를 식별하는데 사용할 카메라 클래스
	const Camera*										    m_pCamera = nullptr;

	//모델 데이터와 애니메이션 데이터
	//fbx 또는 custom format을 읽어들어서 생성한다.
	//생성 시 할당되는 주소 값을 키 값으로 사용
	std::unordered_map<void*, std::shared_ptr<HModel>>		m_models;
	std::unordered_map<void*, std::shared_ptr<HAnim>>		m_anims;
	
	//instance data//전체 인스턴스 숫자와 카메라 범위 안의 인스턴스 숫자
	std::unordered_map<void*, std::weak_ptr<HInstance>>		m_instances;
	std::vector<HInstance*>									m_visibleInstances;
	unsigned int											m_totalInstance = 0;
	unsigned int											m_visibleInstance = 0;

	//모든 모델들의 버텍스와 인덱스를 병합한 데이터
	std::vector<HVertex>									m_allVertices;
	std::vector<std::uint32_t>								m_allIndices;
	ComPtr<ID3D12Resource>									m_allVerticesResource;
	ComPtr<ID3D12Resource>									m_allIndicesResource;
	UINT													m_allVerticesCount = 0;
	UINT													m_allIndicesCount = 0;

	//레이트레이싱 쉐이더에서 쓰일 TLAS와 structured buffer들
	//(raytracing 특성상 모든 인스턴스들의 정보들을 한번에 Set해줘야 하기 때문에 Model이나 Instance 단이 아닌
	//Manager 단에서 이 버퍼를 갖고 있는다.)
	ComPtr<ID3D12Resource>									m_pTopLevelAccelerationStructures;
	ComPtr<ID3D12Resource>									m_pTopLevelAccelerationStructureScratches;
	SharedGraphicsResource									m_pRaytracingInstanceDescs;
	
	SharedGraphicsResource									m_raytracingStructuredBuffer_perInstance;
	SharedGraphicsResource									m_raytracingStructuredBuffer_perModel;

	//Bone이 있는 모델들의 각자 애니메이션 종류, 시간에 따라 다르게 인스턴스된 버텍스들
	//Raytracing을 위한 리소스
	//*Raytracing은 Rasterize와 다르게 쉐이더에 진입하기 전에 최종 Geometry를 넘겨줘야 한다.
	//*즉 애니메이션된 새로운 버텍스를 버퍼로 만들어서 BLAS 업데이트에 사용해야 한다.
	ComPtr<ID3D12Resource>									m_pAllDynamicInstacedVertices;

	//위 리소스를 생성해주기 위한 Compute Shader 자원들
	ComPtr<ID3D12PipelineState>								m_pComputePSO;
	ComPtr<ID3D12RootSignature>								m_pComputeShaderRootSignature;


	//모델 더티 플래그, 새로운 모델을 로드 했을 때 true로 전환, 모든 버텍스를 담고 있는 버퍼를 갱신해줄 때 사용
	bool													m_bModelDirty = false;
	bool													m_bRaytracingDirty = false;

public:
	static HModelManager* GetInstance();
	void Initialize(DeviceResources* pDeviceResources, GraphicsMemory* pGraphicMemory, Camera* pCamera);

	unsigned int GetTotalInstanceCnt() const{ return m_instances.size(); }
	unsigned int GetVisibleInstanceCnt() const{ return m_visibleInstance; }
	
	//1. 새로운 모델이 로드되면 전체 버텍스 버퍼를 갱신 
	//2. 모델들의 인스턴스 살아있는지 확인 
	//3. 카메라의 뷰 프러스텀(또는 박스)에 인스턴스가 들어있는지 확인하고 들어와있는 Instance들의 목록을 갱신. 
	void Update();

	//1. Rasterize의 Instancing Draw를 위한 PSO별, 모델별, 인스턴스별 Structured Buffer를 생성해준다.
	void UpdateResource_raterize();
	
	//1. BLAS를 업데이트하기 위한 모든 Dynamic Instanced vertex들을 담을 수 있는 버퍼를 생성
	//2. 위 버퍼를 Compuste Shader에서 업데이트 할 때 사용할 Structured Buffer(모델별 뼈 개수, 버텍스 개수 등)을 생성
	//3. Raytracing pipe line에서 사용할 Structured Buffer들 생성
	void UpdateResouce_raytracing();

	//1. Rasterize와 Raytracing에서 공통으로 사용될 애니메이션된 Bone정보들을 애니메이션 종류, 시간에 따라 업데이트
	void UpdateAllInstancedBoneAnimResource_common();

	//m_allVerticesResource, m_allIndicesResource를 사용하기 위한 View들
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()const;
	D3D12_INDEX_BUFFER_VIEW	 GetIndexBufferView()const;

	//fbx 또는 자체 포맷을 읽어들여 모델 또는 애니메이션 데이터를 생성해주는 함수들 
	HModelData* CreateModelFromFbxFile(std::string fbxName, ResourceUploadBatch& resourceBatch);
	HAnimData* CreateAnimationFromFbxFiles(std::vector<std::string> fbxNames);
	HModelData* CreateModelFromHModelFile(std::string fbxName, ResourceUploadBatch& resourceBatch);
	HAnimData* CreateAnimationFromHAnimFiles(std::vector<std::string> fbxNames);

private:
	//Import 된 fbx를 통해 모델 데이터를 생성
	void CreateModelFromLoadedHModelFile(HModelFormat* pHmodel, HModel* pModel, ResourceUploadBatch& resourceBatch);
	//Import 된 hmodel(자체포맷) 파일을 통해 애니메이션 데이터 생성
	void CreateModelFromImportedFbx(HFbxImporter* pFbxImporter, ResourceUploadBatch& resourceBatch, HModel* pModel);
	void CreateAllMeshData(MeshData* pFbxMeshNode, HMesh* pHNode);
	void LinkMeshChild(MeshData* pFbxMeshNode, HMesh* pHNode, HModel* pModel);
	void CreateAllBoneData(BoneData* pFbxBoneNode, HBone* pHBone);
	void LinkBoneChild(BoneData* pFbxBoneNode, HBone* pHBone, HModel* pModel);
	void PickZeroDepthMeshBone(HFbxImporter* pFbxImporter, HModel* pModel);
	void CreateRaytracingResource(HModel* pModel, ResourceUploadBatch& resourceBatch);
	void CreateRaytracingGeometryDesc(HModel* pModel, D3D12_RAYTRACING_GEOMETRY_DESC& rtDescs);

	//Import 된 fbx를 통해 애니메이션 데이터를 생성
	void CreateAnimationFromImportedFbx(HFbxImporter* pFbxImporter, HAnim* pAnim);
	void CreateAnimDataFromImportedFbx(HBoneAnims* pHAnim, HFbxImporter* pFbxImporter);
	//Import 된 hanim(자체포맷) 파일을 통해 애니메이션 데이터 생성
	void CreateAnimDataFromLoadedHAnimFile(HBoneAnims* pHAnim, HAnimFormat* pHAnimFormat);

	//모델 매니저 업데이트 함수들
	void CheckInstanceAlive();
	void MergeVertexIndexData();
	void CheckFrustumCulling();
	void CalNodeVertexIndexOffsetRecursively(HMesh* pNode, UINT ModelVertexOffset, UINT ModelIndexOffset);
	
	//본 애니메이션 매트릭스 계산 함수들(래스터라이즈, 레이트레이싱 공통)
	void UpdateInstanceBoneAnimResource_common(std::vector<Matrix>& allBoneTM, HInstance* pHinstance, HModel* pHModel);
	void UpdateBoneAnimRecusively_local_common(std::vector<Matrix>& allBoneData, std::unordered_map<std::string, Matrix>& instanceBoneTM,
		HBone* pBone, HBoneAnims* pAnim, HBoneAnims* pSecondAnim, float animTime, float animTimeSecond,float lerpValue, Matrix parentTM);
	void CalculateBoneAnim(Matrix* boneLocalTM, HBoneAnim const* pBoneAnim, float time, Vector3& outScale, Quaternion& outQuat, Vector3& outTrans);

	//래스터라이즈 업데이트 함수들
	void UpdatePerNodeResouceRecursively_rasterize(HMesh* pNode, UINT modelBoneCount);
	void UpdatePerModelResouce_rasterize(HModel* pHModel);
	void UpdatePerInstanceResouce_rasterize(HModel* pHModel, PSOTYPE psoType);
	void UpdatePerPSOResource_rasterize(HModel* pHModel, PSOTYPE psoType, UINT& instanceOffset);
	void UpdatePerInstanceStructuredBuffer_onlyVisible_rasterize(/*std::unordered_map<void*, std::shared_ptr<HInstance>> const& instances*/
		std::vector<HInstance*>& instances,
		SharedGraphicsResource* pStructuredBuffer);

	//레이트레이싱 업데이트 함수들
	void UpdatePerModelResource_raytracing();
	void UpdatePerInstanceResource_raytracing();
	void UpdatePerNodeResource_raytracing();
	void UpdatePerNodeResourceRecusively_raytracing(HMesh* pNode,Matrix AxisChange, std::vector<HSturecturedBuffer_Raytracing_PerNode>& perNode);
	void UpdateRaytracingBottomLevelAccelerationStructure_raytracing(ID3D12GraphicsCommandList4* cmdList);
	void CreateBLAS(ID3D12GraphicsCommandList4* cmdList, HInstance* pHInstance, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc);
	void UpdateBLAS(ID3D12GraphicsCommandList4* cmdList, HInstance* pHInstance, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc);
	void UpdateRaytracingTopLevelAccelerationStructure_raytracing(ID3D12GraphicsCommandList4* cmdList);

	//m_pAllDynamicInstacedVertices를 생성해주기 위한 Compute Shader 자원들 생성
	//Initialize 단계에서 콜
	void CreateComputePSO_lightCalculation();
	void CreateComputeRootSignature();

	//m_pAllDynamicInstacedVertices를 생성해주기 위한 Compute Shader 업데이트 및 실행 함수들
	//매 프레임 콜
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