#pragma once
#include"HSimplePrimitiveStructs.h"

using namespace DX;
using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

namespace DX
{
	class DeviceResources;
}

class HPassConstant;

struct HProceduralGeometry_line;
struct HProceduralGeometry_rect;
struct HSimplePrimitive;
struct HSimplePrimitive_HEngine;

class HSimplePrimitiveManager
{
private:
	const DeviceResources* m_pDeviceResources;
	const HPassConstant* m_pPassConstant;
	const Camera* m_pCamera;

	unique_ptr<PrimitiveBatch<VertexPositionColor>> m_primitivebatch;


	unordered_map<unsigned char, unique_ptr<BasicEffect>> m_lineEffect;
	unordered_map<unsigned char, unique_ptr<BasicEffect>> m_rectEffect;
	unordered_map<unsigned char, unique_ptr<BasicEffect>> m_primitiveEffect;

	unordered_map<unsigned char, unordered_map<void*, unique_ptr<HProceduralGeometry_line_HEngine>>> m_lines;
	unordered_map<unsigned char, unordered_map<void*, unique_ptr<HProceduralGeometry_rect_HEngine>>> m_rects;
	unordered_map<unsigned char, unordered_map<void*,  unique_ptr<HSimplePrimitive_HEngine>>> m_simplePrimitives;



public:
	static HSimplePrimitiveManager* GetInstance();
	void Initialize(DeviceResources* pDeviceResources, HPassConstant* pPassConstant, Camera* pCamera);
	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	void Update();

	void Draw(ID3D12GraphicsCommandList* commandList);

	HProceduralGeometry_line* CreateLine(unsigned char flag);
	HProceduralGeometry_rect* CreateRect(unsigned char flag);
	HSimplePrimitive* CreateBox(Vector3 Size, unsigned char flag);



private:
	HSimplePrimitiveManager() {};

};
