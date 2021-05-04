#pragma once
#include "pch.h"
//#include <vector>
using namespace DirectX;
using namespace DirectX::SimpleMath;

struct Light
{
	Vector3 Strength = { 0.5f, 0.5f, 0.5f };
	float FalloffStart = 1.0f;                          // point/spot light only
	Vector3 Direction = { 0.0f, -1.0f, 0.0f };			// directional/spot light only
	float FalloffEnd = 10.0f;                           // point/spot light only
	Vector3 Position = { 0.0f, 0.0f, 0.0f };			// point/spot light only
	float SpotPower = 64.0f;                            // spot light only
};

#define MaxLights 16
   
struct PassConstants
{
	Matrix View = Matrix();
	Matrix InvView = Matrix();
	Matrix Proj = Matrix();
	Matrix InvProj = Matrix();
	Matrix ViewProj = Matrix();
	Matrix InvViewProj = Matrix();
	Vector3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	Vector2 RenderTargetSize = { 0.0f, 0.0f };
	Vector2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	Vector4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light Lights[MaxLights];
};

struct RaytracingDataPerModel
{
	UINT indexOffset;
	UINT vertexOffset;
};

struct RaytracingDataPerInstance
{
	UINT indexOffset;
	UINT vertexOffset;
	UINT matIndex0;
	UINT matIndex1;
	float lerpValue;
	int	padding[3];
};

struct Vertex
{
	Vector3 Pos;
	Vector3 Normal;
	Vector3 TangentU;
	Vector3 BiNormV;
	Vector2 TexC;

};


