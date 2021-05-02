#pragma once
#include"HPassConstantDefines.h"

class Camera;
namespace DX
{
	class StepTimer;
	class DeviceResources;
}
class ModelFbx;

class HPassConstant
{
	const DeviceResources* m_pDeviceResources;
	GraphicsMemory* m_pGraphicMemory;

	HPassConstants							 m_passConstants;
	SharedGraphicsResource					 m_passConstantBuffer;
	SharedGraphicsResource					 m_lightBuffer;

	std::unordered_map<void*, std::unique_ptr<HLight>> m_directLight;
	std::unordered_map<void*, std::unique_ptr<HLight>> m_pointLight;
	std::unordered_map<void*, std::unique_ptr<HLight>> m_spotLight;

	//shadow
	DirectX::BoundingSphere mSceneBounds;

public:
	void Initialize(DeviceResources* pDeviceResources, GraphicsMemory* pGraphicMemory);
	void Update(const Camera& camera, const StepTimer& timer);
	void UpdateLightBuffer();
	void UpdateShadowTransform(const Camera& camera);

	HLightData* AddLight(LightType lightType);

	D3D12_GPU_VIRTUAL_ADDRESS GetPassConstantGpuAddress()const{ return m_passConstantBuffer.GpuAddress(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetLightGpyAddress()const 
	{
		if (!m_lightBuffer)
			return NULL;

		 return m_lightBuffer.GpuAddress();
	}
	friend class HRasterize;
};