#include"pch.h"
#include"HEngine.h"
#include <Dbt.h>
#include"FindMedia.h"
#include"d3dUtil.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

//HDEVNOTIFY g_hNewAudio = nullptr;
HEngine* HEngine::m_app = nullptr;

HEngine* HEngine::GetApp()
{
	if (m_app == nullptr)
		m_app = new HEngine;
	return m_app;
}

HEngine::HEngine()
{
	//하나의 엔진만이 존재할 수 있다.
	assert(m_app == nullptr);

	m_pDeviceResources = DeviceResources::GetInstance();
	m_pDeviceResources->Initialize(
		c_backBufferFormat,
		c_depthBufferFormat, /* If we were only doing MSAA rendering, we could skip the non-MSAA depth/stencil buffer with DXGI_FORMAT_UNKNOWN */
		c_backBufferCount,
		c_minFeatureLevel,
		DX::DeviceResources::c_AllowTearing | DX::DeviceResources::c_EnableHDR
	);
	
	m_pDeviceResources->RegisterDeviceNotify(this);
	//
	// In Win32 'classic' DirectX 11, you can create the 'swapchain' backbuffer as a multisample buffer.  Present took care of the
	// resolve as part of the swapchain management.  This is not recommended as doing it explictly gives you more control
	// as well as the fact that this 'old-school' implicit resolve behavior is not supported for UWP.
	//
}

HEngine::~HEngine()
{
	if (m_pDeviceResources)
	{
		m_pDeviceResources->WaitForGpu();
	}
}

void HEngine::Initialize(HWND hwnd, int width, int height)
{
	//해당객체에 윈도우 핸들과 클라이언트 크기를 셋해준다.
	m_pDeviceResources->SetWindow(hwnd, width, height);

	m_pDeviceResources->CreateDeviceResources();			//디바이스 등의 기본적인 자원들을 생성
	CreateDeviceDependentResources();						//생성한 디바이스를 통해서 다른 자원들을 생성
	
	//클라이언트 크기에 따라 변하는 자원들을 생성
	//해당 크기는 런타임 중에도 변할 수 있다. 그럴 경우 아래 함수를 다시 호출한다.
	m_pDeviceResources->CreateWindowSizeDependentResources();//스왑체인을 생성
	CreateWindowSizeDependentResources();					//Proj mat, view port 등 생성

	m_camera.SetPosition(0.0f, 0.0f, -50.0f);
}

#pragma region WindowCreate & Message Handlers 
void HEngine::OnActivated()
{

}

void HEngine::OnDeactivated()
{

}

void HEngine::OnSuspending()
{

}

void HEngine::OnResuming()
{
	m_timer.ResetElapsedTime();
}

void HEngine::OnWindowSizeChanged(int width, int height)
{
	if (!m_pDeviceResources->WindowSizeChanged(width, height))
		return;

	CreateWindowSizeDependentResources();
}

void HEngine::GetDefaultSize(int& width, int& height) const
{
	width = 1280;
	height = 720;
}
#pragma endregion

#pragma region Frame Update

void HEngine::Tick()
{
	m_timer.Tick([&]()
		{
			Update(m_timer);
		});

	Render();
}

void HEngine::Update(DX::StepTimer const& timer)
{
	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");
	//카메라 업데이트
	m_camera.UpdateViewMatrix();

	m_pModelManager->Update();

	m_passConstant.Update(m_camera, timer);

	m_graphicPipeline.Update(timer.GetElapsedSeconds());

	PIXEndEvent();
}

#pragma endregion

#pragma region Frame Render

void HEngine::Render()
{
	auto commandList = m_pDeviceResources->GetCommandList();
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}

	// Prepare the command list to render a new frame.
	m_pDeviceResources->Prepare();

	//render
	m_graphicPipeline.Draw();

	// Show the new frame.
	PIXBeginEvent(m_pDeviceResources->GetCommandQueue(), PIX_COLOR_DEFAULT, L"Present");
	m_pDeviceResources->Present();
	m_pHBufferManager->Commit(m_pDeviceResources->GetCommandQueue());
	m_graphicsMemory->Commit(m_pDeviceResources->GetCommandQueue());
	PIXEndEvent(m_pDeviceResources->GetCommandQueue());
}
#pragma endregion

#pragma region Direct3D Resources

void HEngine::CreateDeviceDependentResources()
{
	auto device = m_pDeviceResources->GetD3DDevice();

	m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

	m_textureManager = HTextureManager::GetInstance();
	m_textureManager->Initialize(device);

	m_pModelManager = HModelManager::GetInstance();
	m_pModelManager->Initialize(m_pDeviceResources, m_graphicsMemory.get(), &m_camera);
	m_passConstant.Initialize(m_pDeviceResources, m_graphicsMemory.get());
	m_graphicPipeline.Initialize(m_graphicsMemory.get(), m_pDeviceResources, m_pModelManager, &m_passConstant, m_textureManager, &m_camera, &m_timer);
	m_picking.Initialize(m_pDeviceResources, m_pModelManager, &m_camera);
	m_pHBufferManager = HBufferManager::GetInstance();

	//GPU를 통한 리소스 생성에 필요한 객체(commandList, commandAlloc 등이 내재되어있다.)
	ResourceUploadBatch resourceUpload(device);
	//Command list 작성 시작
	resourceUpload.Begin();
	m_graphicPipeline.CreateDeviceDependentResources(resourceUpload);
	//Command list 작성 완료
	auto uploadResourcesFinished = resourceUpload.End(m_pDeviceResources->GetCommandQueue());
	//GPU가 모든 명령어를 실행하기를 기다린다.
	uploadResourcesFinished.wait();
}

void HEngine::CreateWindowSizeDependentResources()
{
	auto size = m_pDeviceResources->GetOutputSize();

	//float aspectRatio = float(size.right) / float(size.bottom);
	//float fovAngleY = 60.0f * XM_PI / 180.0f;

	//// This is a simple example of change that can be made when the app is in
	//// portrait or snapped view.
	//if (aspectRatio < 1.0f)
	//{
	//	fovAngleY *= 2.0f;
	//}

	//m_camera.SetLens(fovAngleY, size.right, size.bottom, 1.0f, 1800.f);


	m_graphicPipeline.CreateWindowSizeDependetResources();

}

void HEngine::OnDeviceLost()
{
	m_graphicsMemory.reset();
}

void HEngine::OnDeviceRestored()
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
#pragma endregion

#pragma region HEngine_DX12_3D

void HEngine::StartSetting()
{
	m_resourceUploadBatch.reset(new ResourceUploadBatch(m_pDeviceResources->GetD3DDevice()));
	m_resourceUploadBatch->Begin();
}

void HEngine::FinishSetting()
{
	//Command list 작성 완료
	auto uploadResourcesFinished = m_resourceUploadBatch->End(m_pDeviceResources->GetCommandQueue());

	//GPU가 모든 명령어를 실행하기를 기다린다.
	uploadResourcesFinished.wait();

	m_resourceUploadBatch.reset();
}

void HEngine::WaitGPU()
{
	m_pDeviceResources->WaitForGpu();
}

float HEngine::GetElapsedTime()
{
	return m_timer.GetElapsedSeconds();
}

void HEngine::SetCamera(Camera camera)
{
	m_camera = camera;
}

Camera* HEngine::GetCamera()
{
	return &m_camera;
}

HInstanceData* HEngine::Picking(unsigned int pickX, unsigned int pickY)
{
	return m_picking.Update(pickX, pickY);
}

float HEngine::GetOrthoCameraPickingDepth(int screenX, int screenY)
{
	return m_picking.GetOrthoCameraPickingDepth(screenX, screenY);
}

void HEngine::ProcessWndMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	static bool s_in_sizemove = false;
	static bool s_in_suspend = false;
	static bool s_minimized = false;
	static bool s_fullscreen = false;
	// Set s_fullscreen to true if defaulting to fullscreen.

	if (m_app == nullptr)
		return;

	switch (message)
	{
	case WM_PAINT:
		if (s_in_sizemove && m_app)
		{
			m_app->Tick();
		}
		else
		{
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		break;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			if (!s_minimized)
			{
				s_minimized = true;
				if (!s_in_suspend && m_app)
					m_app->OnSuspending();
				s_in_suspend = true;
			}
		}
		else if (s_minimized)
		{
			s_minimized = false;
			if (s_in_suspend && m_app)
				m_app->OnResuming();
			s_in_suspend = false;
		}
		else if (!s_in_sizemove && m_app)
		{
			m_app->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
		}
		break;

	case WM_ENTERSIZEMOVE:
		s_in_sizemove = true;
		break;

	case WM_EXITSIZEMOVE:
		s_in_sizemove = false;
		if (m_app)
		{
			RECT rc;
			GetClientRect(hWnd, &rc);

			m_app->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
		}
		break;

	case WM_GETMINMAXINFO:
	{
		auto info = reinterpret_cast<MINMAXINFO*>(lParam);
		info->ptMinTrackSize.x = 320;
		info->ptMinTrackSize.y = 200;
	}
	break;

	case WM_ACTIVATEAPP:
		if (m_app)
		{
			Keyboard::ProcessMessage(message, wParam, lParam);
			Mouse::ProcessMessage(message, wParam, lParam);

			if (wParam)
			{
				m_app->OnActivated();
			}
			else
			{
				m_app->OnDeactivated();
			}
		}
		break;

	case WM_POWERBROADCAST:
		switch (wParam)
		{
		case PBT_APMQUERYSUSPEND:
			if (!s_in_suspend && m_app)
				m_app->OnSuspending();
			s_in_suspend = true;
			break;

		case PBT_APMRESUMESUSPEND:
			if (!s_minimized)
			{
				if (s_in_suspend && m_app)
					m_app->OnResuming();
				s_in_suspend = false;
			}
			break;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
		{
			// Implements the classic ALT+ENTER fullscreen toggle
			if (s_fullscreen)
			{
				SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

				int width = 800;
				int height = 600;
				if (m_app)
					m_app->GetDefaultSize(width, height);

				ShowWindow(hWnd, SW_SHOWNORMAL);

				SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}
			else
			{
				SetWindowLongPtr(hWnd, GWL_STYLE, 0);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

				ShowWindow(hWnd, SW_SHOWMAXIMIZED);
			}

			s_fullscreen = !s_fullscreen;
		}
		break;

		//case WM_MENUCHAR:
		//	// A menu is active and the user presses a key that does not correspond
		//	// to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
		//	return MAKELRESULT(0, MNC_CLOSE);
	}
}


void HEngine::AddDebugString(DebugString debugString)
{
	HUIManager::GetInstance()->AddDebugString(debugString);
}

HProceduralGeometry_line* HEngine::CreateLine(unsigned char flag)
{
	return HSimplePrimitiveManager::GetInstance()->CreateLine(flag);
}

HProceduralGeometry_rect* HEngine::CreateRect(unsigned char flag)
{
	return HSimplePrimitiveManager::GetInstance()->CreateRect(flag);
}

HSimplePrimitive* HEngine::CreateBox(Vector3 Size, unsigned char flag)
{
	return HSimplePrimitiveManager::GetInstance()->CreateBox(Size, flag);
}

HMaterialData* HEngine::CreateMaterial(const WCHAR* albedo, const WCHAR* roughness, const WCHAR* metallic, const WCHAR* ao, const WCHAR* normal, const WCHAR* height, const WCHAR* emissive)
{
	return HTextureManager::GetInstance()->CreateMaterial(*m_resourceUploadBatch,
		albedo, roughness, metallic, ao, normal, height, emissive);
}

HModelData* HEngine::CreateModelFromFbxFile(std::string fbxfile)
{
	return m_pModelManager->CreateModelFromFbxFile(fbxfile, *m_resourceUploadBatch);
}

HAnimData* HEngine::CreateAnimationFromFbxFiles(std::vector<std::string> fbxNames)
{
	return m_pModelManager->CreateAnimationFromFbxFiles(fbxNames);
}

HModelData* HEngine::CreateModelFromHModelFile(std::string fbxfile)
{
	return m_pModelManager->CreateModelFromHModelFile(fbxfile, *m_resourceUploadBatch);
}

HAnimData* HEngine::CreateAnimationFromHAnimFiles(std::vector<std::string> fbxNames)
{
	return m_pModelManager->CreateAnimationFromHAnimFiles(fbxNames);
}

HLightData* HEngine::CreateLight(LightType lightType)
{
	return m_passConstant.AddLight(lightType);
}

HSpriteData* HEngine::CreateSprite(const WCHAR* spriteFile)
{
	return HTextureManager::GetInstance()->CreateSprite(*m_resourceUploadBatch,
		spriteFile);
}

HUIData* HEngine::CreateUI()
{
	return HUIManager::GetInstance()->CreateUI();
}

HWaveData* HEngine::CreateWave(int m, int n, float dx, float speed, float damping)
{
	return HWaveEffect::GetInstance()->CreateWave(m, n, dx,0.03f,speed,damping);
}

void HEngine::LoadSkyBox(const WCHAR* skyboxDDSFile)
{
	m_textureManager->LoadSkybox(*m_resourceUploadBatch, skyboxDDSFile);
}

void HEngine::LoadColorChip(const WCHAR* baseColor, const WCHAR* roughness, const WCHAR* metallic, const WCHAR* emissive)
{
	m_textureManager->LoadColorChip(*m_resourceUploadBatch, baseColor,
		roughness, metallic, emissive);
}

void HEngine::LoadFont(const WCHAR* spriteFontFile)
{
	HUIManager::GetInstance()->LoadFont(spriteFontFile, *m_resourceUploadBatch);
}

void HEngine::SetReflectionEffect(bool bOnOff)
{
	m_graphicPipeline.SetReflection(bOnOff);
}

void HEngine::SetShadowEffect(bool bOnOff)
{
	m_graphicPipeline.SetShadow(bOnOff);
}

void HEngine::SetSSAO(bool bOnOff)
{
	m_graphicPipeline.SetSSAO(bOnOff);
}

void HEngine::SetWireFrame(bool bOnOff)
{
	m_graphicPipeline.SetWireFrame(bOnOff);
}

void HEngine::SetDOF(bool bOnOff)
{
	m_graphicPipeline.SetDOF(bOnOff);
}

void HEngine::SetDOFParams(float focusDepth, float maxBlurDepthGap)
{
	HPostProcess::GetInstance()->SetDOFParams(focusDepth, maxBlurDepthGap);
}

RECT HEngine::GetClientRectFromEngine()
{
	return m_pDeviceResources->GetOutputSize();
}
#pragma endregion







