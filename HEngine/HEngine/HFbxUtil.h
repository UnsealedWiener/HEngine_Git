#pragma once
#include<fbxsdk.h>
#include<DirectXMath.h>
#include<string>

using namespace DirectX;

XMFLOAT4X4 ConvertMatrix(FbxAMatrix fbxMatrix)
{
	 XMFLOAT4X4 matrix(
		(float)fbxMatrix.Get(0, 0), (float)fbxMatrix.Get(0, 1), (float)fbxMatrix.Get(0, 2), (float)fbxMatrix.Get(0, 3),
		(float)fbxMatrix.Get(1, 0), (float)fbxMatrix.Get(1, 1), (float)fbxMatrix.Get(1, 2), (float)fbxMatrix.Get(1, 3),
		(float)fbxMatrix.Get(2, 0), (float)fbxMatrix.Get(2, 1), (float)fbxMatrix.Get(2, 2), (float)fbxMatrix.Get(2, 3),
		(float)fbxMatrix.Get(3, 0), (float)fbxMatrix.Get(3, 1), (float)fbxMatrix.Get(3, 2), (float)fbxMatrix.Get(3, 3));
	 return matrix;
}

XMFLOAT4 ConvertFloat4(FbxVector4 fbxVector4)
{
	XMFLOAT4 vector((float)fbxVector4[0], (float)fbxVector4[1], (float)fbxVector4[2], (float)fbxVector4[3]);
	return vector;
}

XMFLOAT3 ConvertFloat3(FbxVector4 fbxVector4)
{
	XMFLOAT3 vector((float)fbxVector4[0], (float)fbxVector4[1], (float)fbxVector4[2]);
	return vector;
}

XMFLOAT2 ConvertFloat2(FbxVector2 fbxVector2)
{
	XMFLOAT2 vector((float)fbxVector2[0], (float)fbxVector2[1]);
	return vector;
}

std::string GetAttributeTypeName(FbxNodeAttribute::EType type)
{
    switch (type) {
    case FbxNodeAttribute::eUnknown: return "unidentified";
    case FbxNodeAttribute::eNull: return "null";
    case FbxNodeAttribute::eMarker: return "marker";
    case FbxNodeAttribute::eSkeleton: return "skeleton";
    case FbxNodeAttribute::eMesh: return "mesh";
    case FbxNodeAttribute::eNurbs: return "nurbs";
    case FbxNodeAttribute::ePatch: return "patch";
    case FbxNodeAttribute::eCamera: return "camera";
    case FbxNodeAttribute::eCameraStereo: return "stereo";
    case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
    case FbxNodeAttribute::eLight: return "light";
    case FbxNodeAttribute::eOpticalReference: return "optical reference";
    case FbxNodeAttribute::eOpticalMarker: return "marker";
    case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
    case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
    case FbxNodeAttribute::eBoundary: return "boundary";
    case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
    case FbxNodeAttribute::eShape: return "shape";
    case FbxNodeAttribute::eLODGroup: return "lodgroup";
    case FbxNodeAttribute::eSubDiv: return "subdiv";
    default: return "unknown";
    }
}

