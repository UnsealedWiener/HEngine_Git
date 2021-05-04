#pragma once

#include"SimpleMath.h"
 

using namespace DirectX;
using namespace DirectX::SimpleMath;

struct HModel_Vertex
{
	Vector3 pos;
	Vector3 normal;
	Vector3 tangentU;
	Vector3 biNormV;
	Vector2 texC;
	XMUINT4 boneIndex;
	Vector4 blendWeight;
};

struct HModel_Header
{
	unsigned int meshCount;
	unsigned int boneCount;
	unsigned int totalVertexCount;
	unsigned int totalIndexCount;
};

struct HModel_CommonValue
{
	XMFLOAT4X4	 axisSystemModify;
	BoundingBox	 boundingBox;
};

struct HModel_Mesh
{
	char name[256];
	bool		 bIsDepthZero = false;
	unsigned int index;
	unsigned int childCount;
	unsigned int* pChildIndex = nullptr;
};

struct HModel_Bone
{
	char name[256];
	bool		 bIsDepthZero = false;
	unsigned int index;
	unsigned int childCount;
	unsigned int* pChildIndex = nullptr;
	XMFLOAT4X4 invGlobalTTM;
};

struct HModelFormat
{
	HModel_Header header;
	HModel_CommonValue commonValue;
	HModel_Mesh* pMesh = nullptr;
	HModel_Bone*  pBone = nullptr;
	HModel_Vertex* pVertices = nullptr;
	std::uint32_t* pIndices = nullptr;
};

struct HAnim_Header
{
	char name[256];
	float totalTime = 0;
	unsigned int boneCount;
};

struct HAnim_Bone_KeyFrame
{
	float time;
	XMFLOAT3 scale;
	XMFLOAT4 quaternion;
	XMFLOAT3 translation;
};

struct HAnim_Bone
{
	char name[256];
	unsigned int index;
	unsigned int keyCount;
	HAnim_Bone_KeyFrame* pKey = nullptr;
};

struct HAnimFormat
{
	HAnim_Header header;
	HAnim_Bone* pBone;
};
