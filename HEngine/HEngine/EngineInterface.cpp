#include"pch.h"
#include"HEngine.h"
#include"EngineInterface.h"

HEngine* HEngine_DX12_3D::m_pHEngine = nullptr;


HEngine_DX12_3D* HEngine_DX12_3D:: GetInstance()
{
	static HEngine_DX12_3D HEngine;

	return &HEngine;
}

void HEngine_DX12_3D::InitEngine(HWND hWnd, int clientWidth, int clientHeight)
{
	if (m_pHEngine != nullptr)
	{
		HerrorMessage(L"The HEngine has been initialized more than once.")
		throw;
	}
	m_pHEngine =  HEngine::GetApp();
	m_pHEngine->Initialize(hWnd, clientWidth, clientHeight);
	g_hWnd = hWnd;
}

void HEngine_DX12_3D::Loop()
{
	if (m_pHEngine == nullptr)
		throw;
	m_pHEngine->Tick();
}

void HEngine_DX12_3D::EndEngine()
{
	delete m_pHEngine;
	//CoUninitialize();
}

void HEngine_DX12_3D::ProcessWndMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_pHEngine == nullptr)
		throw;
	m_pHEngine->ProcessWndMessage(hWnd, message, wParam, lParam);
}

void HEngine_DX12_3D::StartSetting()
{
	if (m_pHEngine == nullptr)
		throw;

	m_pHEngine->StartSetting();
}

void HEngine_DX12_3D::FinishSetting()
{
	if (m_pHEngine == nullptr)
		throw;

	m_pHEngine->FinishSetting();
}

float HEngine_DX12_3D::GetElapsedTime()
{
	if (m_pHEngine == nullptr)
		throw;

	return m_pHEngine->GetElapsedTime();
}

void HEngine_DX12_3D::SetCamera(Camera camera)
{
	if (m_pHEngine == nullptr)
		throw;

	m_pHEngine->SetCamera(camera);
}

Camera* HEngine_DX12_3D::GetCamera()
{
	if (m_pHEngine == nullptr)
		throw;

	return m_pHEngine->GetCamera();
}

void HEngine_DX12_3D::AddDebugString(DebugString debugString)
{
	if (m_pHEngine == nullptr)
		throw;
	m_pHEngine->AddDebugString(debugString);
}

HInstanceData* HEngine_DX12_3D::Picking(unsigned int pickX, unsigned int pickY)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->Picking(pickX, pickY);
}

float HEngine_DX12_3D::GetOrthoCameraPickingDepth(int screenX, int screenY)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->GetOrthoCameraPickingDepth(screenX, screenY);
}

HProceduralGeometry_line* HEngine_DX12_3D::CreateLine(unsigned char flag)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateLine(flag);
}

HProceduralGeometry_rect* HEngine_DX12_3D::CreateRect(unsigned char flag)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateRect(flag);
}

HSimplePrimitive* HEngine_DX12_3D::CreateBox(Vector3 Size, unsigned char flag)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateBox(Size, flag);
}

HUIData* HEngine_DX12_3D::CreateUI()
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateUI();
}

HLightData* HEngine_DX12_3D::CreateLight(LightType lightType)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateLight(lightType);
}

RECT HEngine_DX12_3D::GetClientRectFromEngine()
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->GetClientRectFromEngine();
}

void HEngine_DX12_3D::LoadSkyBox(const WCHAR* skyBoxDDSFile)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->LoadSkyBox(skyBoxDDSFile);
}

void HEngine_DX12_3D::LoadColorChip(const WCHAR* baseColor, const WCHAR* roughness, const WCHAR* metallic, const WCHAR* emissive)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->LoadColorChip(baseColor,
		roughness, metallic, emissive);
}

void HEngine_DX12_3D::LoadFont(const WCHAR* spriteFontFile)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->LoadFont(spriteFontFile);
}

void HEngine_DX12_3D::SetReflectionEffect(bool bOnOff)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->SetReflectionEffect(bOnOff);
}

HWaveData* HEngine_DX12_3D::CreateWave(int m, int n, float dx, float speed, float damping)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateWave(m, n, dx, speed, damping);
}

void HEngine_DX12_3D::SetShadowEffect(bool bOnOff)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->SetShadowEffect(bOnOff);
}

void HEngine_DX12_3D::SetSSAO(bool bOnOff)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->SetSSAO(bOnOff);
}

void HEngine_DX12_3D::SetWireFrame(bool bOnOff)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->SetWireFrame(bOnOff);
}

void HEngine_DX12_3D::SetDOF(bool bOnOff)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->SetDOF(bOnOff);
}

void HEngine_DX12_3D::SetDOFParams(float focusDepth, float maxBlurDepthGap)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->SetDOFParams(focusDepth, maxBlurDepthGap);
}

void HEngine_DX12_3D::WaitGPU()
{
	if (m_pHEngine == nullptr)
		throw;
	m_pHEngine->WaitGPU();
}

HMaterialData* HEngine_DX12_3D::CreateMaterial(const WCHAR* albedo, const WCHAR* roughness, const WCHAR* metallic, const WCHAR* ao, const WCHAR* normal, const WCHAR* height, const WCHAR* emissive)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateMaterial(albedo, roughness, metallic, ao, normal, height, emissive);
}

HSpriteData* HEngine_DX12_3D::CreateSprite(const WCHAR* spriteFile)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateSprite(spriteFile);
}

HModelData* HEngine_DX12_3D::CreateModelFromFbxFile(std::string fbxName)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateModelFromFbxFile(fbxName);
}

HAnimData* HEngine_DX12_3D::CreateAnimationFromFbxFiles(std::vector<std::string> fbxNames)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateAnimationFromFbxFiles(fbxNames);
}

HModelData* HEngine_DX12_3D::CreateModelFromHModelFile(std::string hmodelFile)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateModelFromHModelFile(hmodelFile);
}

HAnimData* HEngine_DX12_3D::CreateAnimationFromHAnimFiles(std::vector<std::string> hAnimFiles)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateAnimationFromHAnimFiles(hAnimFiles);
}
