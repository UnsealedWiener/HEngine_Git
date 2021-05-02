#pragma once
#include"EngineInterface.h"
#include"HManagerController.h"

using namespace DX;
using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;


class HSimplePrimitiveManager;


struct HProceduralGeometry_line_HEngine : public HProceduralGeometry_line
{
	HManagerController<HSimplePrimitiveManager, unordered_map<void*, unique_ptr<HProceduralGeometry_line_HEngine>>> managerController;
	void Delete()override;
};
struct HProceduralGeometry_rect_HEngine : public HProceduralGeometry_rect
{
	HManagerController<HSimplePrimitiveManager, unordered_map<void*, unique_ptr<HProceduralGeometry_rect_HEngine>>> managerController;
	void Delete()override;
};

struct HSimplePrimitive_HEngine : public HSimplePrimitive
{
	HManagerController<HSimplePrimitiveManager, unordered_map<void*, unique_ptr<HSimplePrimitive_HEngine>>> managerController;
	std::unique_ptr<GeometricPrimitive> geometryPrimitive;
	void Delete()override;
};
