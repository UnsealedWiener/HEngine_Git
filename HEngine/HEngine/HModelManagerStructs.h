#pragma once
#include"EngineInterface.h"
#include"HManagerController.h"
using namespace DX;
using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;


struct HVertex_raytracing
{
	Vector3 pos;
	Vector3 normal;
	Vector3 tangentU;
	Vector3 biNormV;
	Vector2 texC;
	int		padding[2];
};

class HModelManager;
struct HInstance;
struct HInstanceTwoMat;
struct HInstance;
struct HAnim;

struct HModel : public HModelData
{
	HModelRawData rawData;

	std::shared_ptr<HAnim> m_pAnim;

	//offset in all in one buffers//updated when HManager is Updated
	UINT						vertexOffsetInEntireBuffer;
	UINT					    indexOffsetInEntireBuffer;

	//instances data
	std::unordered_map<unsigned char, std::unordered_map<void*, std::shared_ptr<HInstance>>> instances;
	std::unordered_map<unsigned char, std::vector<HInstance*>> visibleInstances;
	UINT visibleInstanceCount = 0;
	UINT visibleInstanceCount_temp = 0;

	//raytracing pipeline resources
	//Used for static model(no animation)
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> indexBuffer;
	ComPtr<ID3D12Resource> bottomLevelAccelerationStructureBuffer;
	ComPtr<ID3D12Resource> bottomLevelAccelerationStructureScratchBuffer;


	//rasterize pipeline resources
	SharedGraphicsResource perModelCB;
	std::unordered_map<unsigned char, SharedGraphicsResource> structuredBuffer_perPso;
	std::unordered_map<unsigned char, SharedGraphicsResource> structuredBuffer_instances;
	//std::unordered_map<unsigned char, SharedGraphicsResource> structuredBuffer_bones;

	//working(for compute shader)
	SharedGraphicsResource structuredBuffer_computeShader_boneTMs;
	SharedGraphicsResource structuredBuffer_computeShader_model;


	HManagerController<HModelManager, std::unordered_map<void*, std::shared_ptr<HModel>>> managerController;
	void Delete()override;
	HInstanceData* AddInstance(unsigned char flag)override;
	void SetAnimation(HAnimData* pAnimData)override;
	HAnimData* GetAnimation()override;

};

struct HAnim : public HAnimData
{
	HAnimRawData rawData;
	HManagerController < HModelManager, std::unordered_map<void*, std::shared_ptr<HAnim>>> managerController;

	void Delete()override;
};



struct HInstance : public HInstanceData
{
	unsigned char m_flag;					//InstanceType

	std::weak_ptr<HModel> pModel;
	std::weak_ptr<UINT> pMat;
	std::weak_ptr<UINT> pMatSecond;

	std::unordered_map<std::string, Matrix> boneTMCurrFrame;//현재 애니메이션 프레임에서의 본들의 TM
	//this is used when the model of the instance is dynamic model.(raytracing)
	//std::vector<ComPtr<ID3D12Resource>> bottomLevelAccelerationStructureBuffer;
	//std::vector<ComPtr<ID3D12Resource>> bottomLevelAccelerationStructureScratchBuffer;
	bool bIsInCameraRange = true;

	ComPtr<ID3D12Resource> bottomLevelAccelerationStructureBuffer;
	ComPtr<ID3D12Resource> bottomLevelAccelerationStructureScratchBuffer;

	HManagerController<HModel, std::unordered_map<void*, std::shared_ptr<HInstance>>> managerController;
	void Delete()override;
	void SetMaterial(HMaterialData* pMaterial, unsigned int slot)override;
	void SetShaderFlag(unsigned char flag)override;
	Matrix GetBoneTMAtCurruntFrame(std::string boneName)override;

};

struct HStructuredBuffer_PerPSO_Rasterize
{
	UINT instanceNumOffset;
	UINT padding[3];
};

struct HStructuredBuffer_PerInstance_Rasterize
{
	Matrix worldTM;
	Matrix textureTM;
	UINT   materialIndex0;
	UINT   materialIndex1;
	FLOAT  materialLerpValue;
	UINT   padding;
};

struct HConstantBuffer_PerModel_Rasterize
{
	Matrix axisChange;
	UINT boneCount;
	UINT padding[3];
};

struct HConstantBuffer_PerNode_Rasterize
{
	Matrix nodeTM;
};

struct HSturecturedBuffer_Raytracing_PerInstance
{
	UINT	modelIndex;
	UINT	instanceIndexInModel;
	UINT	matIndex0;
	UINT	matIndex1;
	FLOAT	lerpValue;
	UINT	padding[3];
};

struct HSturecturedBuffer_Raytracing_PerModel
{
	UINT vertexOffsetInEntireBuffer;
	UINT indexOffsetInEntireBuffer;
	UINT vertexCount;
	BOOL isDynamicModel;
};

struct HSturecturedBuffer_Raytracing_PerNode
{
	Matrix globalTransform;
	UINT indexOffsetInModel;
	UINT vertexOffsetInModel;
	UINT padding[2];
};

enum InstanceType
{
	NOOPTION_MM =	0B00000000,
	TWOMAT_MM	=	0B00000001,
	TESS_MM		=	0B00000010,
	WIREFRAME_MM = 0B00000100,
	ALLOPTION_MM   = TWOMAT_MM| TESS_MM | WIREFRAME_MM
};

enum class RootSig_Compute
{
	StructuredBuffer_VertexInput,
	StructuredBuffer_VertexOutput,
	StructuredBuffer_BoneTM,
	Constant_PerModel,
	MaxCount
};

struct HStructuredBuffer_Raytracing_ComputeShader_PerModel
{
	UINT instanceCount;
	UINT vertexCount;
	UINT boneCount;
	UINT vertexRecordOffset;
};
