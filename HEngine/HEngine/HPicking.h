#pragma once
#include"EngineInterface.h"
class HModelManager;
namespace DX
{
	class DeviceResources;
}
struct HModel;
struct HMesh;


class HPicking
{
	const HModelManager* m_pModelManager;
	const Camera* m_pCamera;
	const DX::DeviceResources* m_pDeviceResources;

	bool m_bIsActivated = false;

public:
	void Initialize(DX::DeviceResources* pDeviceResources, HModelManager* pHModelManager, Camera* pCamera);
	HInstanceData* Update(int screenX, int screenY);

};
