#pragma once
#include"HManagerController.h"
#include "EngineInterface.h"

class HUIManager;

struct HUI : public HUIData
{
	std::weak_ptr<UINT> pTextureIndex;
	HManagerController<HUIManager, std::unordered_map<void*, std::unique_ptr<HUI>>> managerController;
	void Delete()override;
	void SetSprite(HSpriteData* pSprite)override;
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