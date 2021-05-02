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
	CoUninitialize();
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

DirectX::Keyboard* HEngine_DX12_3D::GetKeyboard()
{
	if (m_pHEngine == nullptr)
		throw;

	return m_pHEngine->GetKeyboard();
}

DirectX::GamePad* HEngine_DX12_3D::GetGamePad()
{
	if (m_pHEngine == nullptr)
		throw;

	return m_pHEngine->GetGamePad();
}

DirectX::Mouse* HEngine_DX12_3D::GetMouse()
{
	if (m_pHEngine == nullptr)
		throw;

	return m_pHEngine->GetMouse();
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

void HEngine_DX12_3D::SetReflectionEffect(bool bOnOff)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->SetReflectionEffect(bOnOff);
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

HMaterialData* HEngine_DX12_3D::CreateMaterial(const WCHAR* albedo, const WCHAR* roughness, const WCHAR* metallic, const WCHAR* ao, const WCHAR* normal, const WCHAR* height)
{
	if (m_pHEngine == nullptr)
		throw;
	return m_pHEngine->CreateMaterial(albedo, roughness, metallic, ao, normal, height);
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
