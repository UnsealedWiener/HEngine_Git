#include"pch.h"
#include"HPicking.h"
#include"HModelManager.h"
#include"DeviceResources.h"

void HPicking::Initialize(DX::DeviceResources* pDeviceResources, HModelManager* pHModelManager, Camera* pCamera)
{
	m_pModelManager = pHModelManager;
	m_pCamera = pCamera;
	m_pDeviceResources = pDeviceResources;

}

HInstanceData* HPicking::Update(int screenX, int screenY)
{
	XMFLOAT4X4 ProjectionMat = m_pCamera->GetProj4x4f();

	RECT outputSize = m_pDeviceResources->GetOutputSize();
	float clientWidth = outputSize.right;
	float clientHeight = outputSize.bottom;

	//ray in view space
	float vx = (2.0f * screenX / clientWidth - 1.0f) / ProjectionMat(0, 0);
	float vy = (-2.0f * screenY / clientHeight + 1.0f) / ProjectionMat(1, 1);

	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);


	XMMATRIX ViewMat = m_pCamera->GetView();
	XMVECTOR det = XMMatrixDeterminant(ViewMat);
	XMMATRIX invViewMat = XMMatrixInverse(&det, ViewMat);

	XMVECTOR rayOrigin_world = XMVector3TransformCoord(rayOrigin, invViewMat);
	XMVECTOR rayDir_world = XMVector3TransformNormal(rayDir, invViewMat);

	rayDir_world = XMVector3Normalize(rayDir_world);

	float tmin = FLT_MAX;
	HInstance* pMinRangeInstance = nullptr;
	HModel* pMinRangeModel = nullptr;
	HInstanceData* instanceHandle = 0;
	for (const std::pair<void*, std::weak_ptr<HInstance>>& instancePair : m_pModelManager->m_instances)
	{
		HInstance* pInstance = instancePair.second.lock().get();
		HModel* pModel = pInstance->pModel.lock().get();

		BoundingBox instanceWorldBoundingBox;
		pModel->rawData.boundingBox.Transform(instanceWorldBoundingBox, pInstance->worldTM);

		float tCurrunt = 0;
		if (instanceWorldBoundingBox.Intersects(rayOrigin_world, rayDir_world, tCurrunt))
		{
			if (tCurrunt < tmin)
			{
				tmin = tCurrunt;
				pMinRangeInstance = pInstance;
				pMinRangeModel = pModel;
				instanceHandle = (HInstanceData*)instancePair.first;
			}
		}
	}

	float t;

	if (pMinRangeInstance == nullptr)
		return 0; 
	else
	{
		XMMATRIX instanceWorldTM = XMLoadFloat4x4(&pMinRangeInstance->worldTM);
		det = XMMatrixDeterminant(instanceWorldTM);
		XMMATRIX invWorld = XMMatrixInverse(&det, instanceWorldTM);

		XMVECTOR rayOrigin_local = XMVector3TransformCoord(rayOrigin_world, invWorld);
		XMVECTOR rayDir_local = XMVector3TransformNormal(rayDir_world, invWorld);

		XMMATRIX axisModify = XMLoadFloat4x4(&pMinRangeModel->rawData.AxisSystempModify);
		det = XMMatrixDeterminant(axisModify);
		XMMATRIX invAxisModify = XMMatrixInverse(&det, axisModify);

		XMVECTOR rayOrigin_3dmaxlocal = XMVector3TransformCoord(rayOrigin_local, invAxisModify);
		XMVECTOR rayDir_3dmaxlocal = XMVector3TransformNormal(rayDir_local, invAxisModify);

		rayDir_3dmaxlocal = XMVector3Normalize(rayDir_3dmaxlocal);

		for (int i = 0; i < pMinRangeModel->rawData.vertexData.size() / 3; i++)
		{
			XMVECTOR v0 = XMLoadFloat3(&pMinRangeModel->rawData.vertexData[3 * i].pos);
			XMVECTOR v1 = XMLoadFloat3(&pMinRangeModel->rawData.vertexData[3 * i + 1].pos);
			XMVECTOR v2 = XMLoadFloat3(&pMinRangeModel->rawData.vertexData[3 * i + 2].pos);

			if (DirectX::TriangleTests::Intersects(rayOrigin_3dmaxlocal, rayDir_3dmaxlocal, v0, v1, v2, t))
			{
				return instanceHandle;
			}
		}
	}

	return 0;

}
