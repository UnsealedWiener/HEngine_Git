#pragma once

#include<unordered_map>

using namespace DirectX;
using namespace DirectX::SimpleMath;

struct HVertex
{
	Vector3 pos;
	Vector3 normal;
	Vector3 tangentU;
	Vector3 biNormV;
	Vector2 texC;
	XMUINT4 boneIndex;
	Vector4 blendWeight;
};

struct HMesh
{
	UINT indexCount;
	UINT vertexCount;
	UINT indexOffset;
	UINT vertexOffset;
	UINT vertexOffsetInEntireBuffer;
	UINT indexOffsetInEntireBuffer;



	//Matrix staticTM;				//애니메이션이 없을 때
	//Matrix globalBindTM;			//스킨 애니메이션이 있을 때 

	//BoundingBox boundingBox;
	//BoundingSphere boundingSphere;

	//working
	std::vector<HMesh*> pChildNode;

	//rasterize resource
	//SharedGraphicsResource perNodeCB;
	std::string name;
};

struct HBone
{
	UINT boneIndex;	//boneTM 상수버퍼 내에서의 인덱스
	Matrix invGlobalTTM;
	Matrix localTTM;
	std::vector<HBone*> pChildBone;

	std::string name;
};

struct HAnimKey
{
	float time;
	Vector3 scale;
	Vector4 quatRot;
	Vector3 trans;
};

struct HBoneAnim
{
	std::string name;
	std::vector<HAnimKey> animKeys;
};

struct HBoneAnims
{
	float totalTime;
	Matrix AxisSystempModify;
	std::vector<HBoneAnim> boneAnim;
};


struct HAnimRawData
{
	std::unordered_map<std::string, std::unique_ptr<HBoneAnims>> animBundle;
};

struct HModelRawData
{
	//raw data
	std::vector<HVertex>		vertexData;
	std::vector<std::uint32_t>	indexData;

	//hierarchy data from fbx file
	std::vector<HMesh*> pDepthZeroMeshNodes;
	std::vector<std::unique_ptr<HMesh>> pAllMeshNodes;
	std::vector<HBone*> pDepthZeroBoneNodes;
	std::vector<std::unique_ptr<HBone>> pAllBoneNodes;

	//std::unordered_map<std::string, std::unique_ptr<HAnim>> animations;

	UINT boneCount = 0;
	Matrix AxisSystempModify;

	BoundingBox boundingBox;
};