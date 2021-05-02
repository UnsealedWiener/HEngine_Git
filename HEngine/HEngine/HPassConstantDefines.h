#pragma once
#include"EngineInterface.h"
#include"HManagerController.h"

using namespace DX;
using namespace DirectX;
using namespace DirectX::SimpleMath;

#define MaxLights 16

class HPassConstant;

struct HLight : public HLightData
{
	//Vector3 strength = { 0.5f, 0.5f, 0.5f };
	//float falloffStart = 1.0f;                          // point/spot light only
	//Vector3 direction = { 0.0f, -1.0f, 0.0f };			// directional/spot light only
	//float falloffEnd = 10.0f;                           // point/spot light only
	//Vector3 position = { 0.0f, 0.0f, 0.0f };			// point/spot light only
	//float spotPower = 64.0f;                            // spot light only
	LightType lightType;
	HManagerController<HPassConstant, std::unordered_map<void*, std::unique_ptr<HLight>>> managerController;
	void Delete()override;

};

struct HLight_StructuredBuffer
{
	Vector3 strength = { 0.5f, 0.5f, 0.5f };
	float falloffStart = 1.0f;                          // point/spot light only
	Vector3 direction = { 0.0f, -1.0f, 0.0f };			// directional/spot light only
	float falloffEnd = 10.0f;                           // point/spot light only
	Vector3 position = { 0.0f, 0.0f, 0.0f };			// point/spot light only
	float spotPower = 64.0f;                            // spot light only
};

struct HPassConstants
{
	Matrix view = Matrix();
	Matrix invView = Matrix();
	Matrix proj = Matrix();
	Matrix invProj = Matrix();
	Matrix viewProj = Matrix();
	Matrix invViewProj = Matrix();
	Matrix lightView = Matrix();
	Matrix lightProj = Matrix();
	Matrix shadowTransform = Matrix();
	Vector3 eyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	Vector2 renderTargetSize = { 0.0f, 0.0f };
	Vector2 invRenderTargetSize = { 0.0f, 0.0f };
	float nearZ = 0.0f;
	float farZ = 0.0f;
	float totalTime = 0.0f;
	float deltaTime = 0.0f;

	Vector4 ambientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	//HLight_StructuredBuffer lights[MaxLights];
	UINT dirlightcnt = 0;
	UINT pointlightcnt = 0;
	UINT spotlightcnt = 0;
	BOOL isPerspective;

};
