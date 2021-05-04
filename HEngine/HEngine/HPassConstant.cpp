#include"pch.h"
#include"HPassConstant.h"
#include"Camera.h"
#include"StepTimer.h"
#include"DeviceResources.h"




void HPassConstant::Initialize(DeviceResources * pDeviceResources, GraphicsMemory * pGraphicMemory)
{
	m_pDeviceResources = pDeviceResources;
	m_pGraphicMemory = pGraphicMemory;

	mSceneBounds.Center = XMFLOAT3(0.f, 0.f, 0.f);
	mSceneBounds.Radius = 1;// sqrtf(300.f * 300.f + 300.f * 300.f);
}

void HPassConstant::Update(const Camera& camera, const StepTimer& timer)
{
	auto output = m_pDeviceResources->GetOutputSize();
	float clientWidth = output.right - output.left;
	float clientHeight = output.bottom - output.top;

	XMMATRIX view = camera.GetView();
	XMMATRIX proj = camera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMVECTOR det = XMMatrixDeterminant(view);
	XMMATRIX invView = XMMatrixInverse(&det, view);
	det = XMMatrixDeterminant(proj);
	XMMATRIX invProj = XMMatrixInverse(&det, proj);
	det = XMMatrixDeterminant(viewProj);
	XMMATRIX invViewProj = XMMatrixInverse(&det, viewProj);

	XMStoreFloat4x4(&m_passConstants.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_passConstants.invView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&m_passConstants.proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_passConstants.invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&m_passConstants.viewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&m_passConstants.invViewProj, XMMatrixTranspose(invViewProj));
	m_passConstants.eyePosW = camera.GetPosition3f();
	m_passConstants.renderTargetSize = XMFLOAT2(clientWidth, clientHeight);
	m_passConstants.invRenderTargetSize = XMFLOAT2(1.0f / clientWidth, 1.0f / clientHeight);
	m_passConstants.nearZ = camera.GetNearZ();
	m_passConstants.farZ = camera.GetFarZ();
	m_passConstants.totalTime = timer.GetTotalSeconds();
	m_passConstants.deltaTime = timer.GetElapsedSeconds();
	m_passConstants.ambientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

	m_passConstants.dirlightcnt = m_directLight.size();
	m_passConstants.pointlightcnt = m_pointLight.size();
	m_passConstants.spotlightcnt = m_spotLight.size();

	if (camera.GetCamMode() == CameraMode::ePerspective)
		m_passConstants.isPerspective = TRUE;
	else
		m_passConstants.isPerspective = FALSE;

	UpdateShadowTransform(camera);

	m_passConstantBuffer.Reset(m_pGraphicMemory->AllocateConstant(m_passConstants));
	
	UpdateLightBuffer();
}

void HPassConstant::UpdateLightBuffer()
{
	std::vector<HLight_StructuredBuffer> lightDatas;

	HLight_StructuredBuffer lightData;

	for (auto& light : m_directLight)
	{
		HLight* pLight = light.second.get();

		lightData.strength = pLight->strength;
		lightData.falloffStart = pLight->falloffStart;
		pLight->direction.Normalize(lightData.direction);
		lightData.falloffEnd = pLight->falloffEnd;
		lightData.position = pLight->position;
		lightData.spotPower = pLight->spotPower;
		lightDatas.push_back(lightData);
	}

	for (auto& light : m_pointLight)
	{
		HLight* pLight = light.second.get();

		lightData.strength = pLight->strength;
		lightData.falloffStart = pLight->falloffStart;
		lightData.direction = pLight->direction;
		lightData.falloffEnd = pLight->falloffEnd;
		lightData.position = pLight->position;
		lightData.spotPower = pLight->spotPower;
		lightDatas.push_back(lightData);
	}

	for (auto& light : m_spotLight)
	{
		HLight* pLight = light.second.get();

		lightData.strength = pLight->strength;
		lightData.falloffStart = pLight->falloffStart;
		lightData.direction = pLight->direction;
		lightData.falloffEnd = pLight->falloffEnd;
		lightData.position = pLight->position;
		lightData.spotPower = pLight->spotPower;
		lightDatas.push_back(lightData);
	}

	if (lightDatas.size() != 0)
	{
		UINT bufferSizePerLight = sizeof(HLight_StructuredBuffer);

		SharedGraphicsResource graphicResource =
			m_pGraphicMemory->Allocate(lightDatas.size() * bufferSizePerLight);

		memcpy(graphicResource.Memory(),
			lightDatas.data(),
			lightDatas.size() * bufferSizePerLight);

		m_lightBuffer.Reset(graphicResource);

	}
}

void HPassConstant::UpdateShadowTransform(const Camera& camera)
{
	static BoundingSphere staticSphere;

	staticSphere.Center = XMFLOAT3(0.f, 0.f, 0.f);
	staticSphere.Radius = 1;// sqrtf(300.f * 300.f + 300.f * 300.f);

	Ray ray(camera.GetPosition(), camera.GetLook3f());
	Plane plane(Vector3(0, 0, 0), Vector3(0, 1, 0));
	
	float dist;
	ray.Intersects(plane, dist);

	Vector3 newPostion = Vector3(camera.GetPosition()) + 
		(Vector3(camera.GetLook3f()) * dist);

	float scale = camera.GetFarWindowWidth() / 3;

	staticSphere.Transform(mSceneBounds, scale, XMVectorSet(0, 0, 0, 1), newPostion);

	XMVECTOR lightDir = XMVectorSet(0.57735f, -0.57735f, 0.57735f, 0.f);
	XMVECTOR lightPos = -1.0f * mSceneBounds.Radius * lightDir + mSceneBounds.Center;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	float l = sphereCenterLS.x - mSceneBounds.Radius*2/3;
	float b = sphereCenterLS.y - mSceneBounds.Radius*2/3;
	float n = sphereCenterLS.z - mSceneBounds.Radius*2/3;
	float r = sphereCenterLS.x + mSceneBounds.Radius*2/3;
	float t = sphereCenterLS.y + mSceneBounds.Radius*2/3;
	float f = sphereCenterLS.z + mSceneBounds.Radius*2/3;

	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, 1, f);

	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S = lightView * lightProj * T;
	XMStoreFloat4x4(&m_passConstants.lightView, XMMatrixTranspose(lightView));
	XMStoreFloat4x4(&m_passConstants.lightProj, XMMatrixTranspose(lightProj));
	XMStoreFloat4x4(&m_passConstants.shadowTransform, XMMatrixTranspose(S));
	//XMStoreFloat4x4(&m_passConstants.lightView,lightView);
	//XMStoreFloat4x4(&m_passConstants.lightProj, lightProj);
	//XMStoreFloat4x4(&m_passConstants.shadowTransform, S);


}

HLightData* HPassConstant::AddLight(LightType lightType)
{
	std::unique_ptr<HLight> pHLight = std::make_unique<HLight>();
	pHLight->lightType = lightType;

	HLightData* pLightData = pHLight.get();

	switch (pHLight->lightType)
	{
	case eDirectLight:
		pHLight->managerController = HManagerController(this, &m_directLight, pLightData);
		m_directLight[pLightData] = std::move(pHLight);
		break;
	case ePointLight:
		pHLight->managerController = HManagerController(this, &m_pointLight, pLightData);
		m_pointLight[pLightData] = std::move(pHLight);
		break;
	case eSpotLight:
		pHLight->managerController = HManagerController(this, &m_spotLight, pLightData);
		m_spotLight[pLightData] = std::move(pHLight);
		break;
	default:
		break;
	}

	return pLightData;
}

void HLight::Delete()
{
	managerController.DeleteMe();
}



