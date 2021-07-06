#pragma once
#include"HManagerController.h"
#include "EngineInterface.h"
#include<map>
class HUIManager;

struct HUI : public HUIData
{
	bool bDrawOnOff = true;
	std::weak_ptr<UINT> pTextureIndex;
	HManagerController_vector<HUIManager,  std::unique_ptr<HUI>> managerController;
	//HManagerController<HUIManager, std::vector<std::unique_ptr<HUI>>> managerController;

	void Delete()override;
	void SetSprite(HSpriteData* pSprite)override;
	void SetDrawOnOff(bool onOff)override;
};

enum class RootSig_UI
{
	eStructuredBufferPerInstance,
	eTextures,
	TotalCount
};

struct HVertex_UI
{
	Vector3 postion;
	Vector2 uv;
};

struct HStructuredBuffer_PerInstance_UI
{
	Matrix worldProjTM;
	UINT   textureIndex;
	FLOAT  opacity;
	FLOAT  padding[2];
};