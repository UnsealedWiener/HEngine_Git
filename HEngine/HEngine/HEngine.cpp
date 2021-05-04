#include"pch.h"
#include"HEngine.h"
#include <Dbt.h>
#include"FindMedia.h"
#include"d3dUtil.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

HDEVNOTIFY g_hNewAudio = nullptr;
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

//윈도우 런타임 가동, 글로벌로 선언해야함.
Microsoft::WRL::Wrappers::RoInitializeWrapper runtimeInit(RO_INIT_MULTITHREADED);

void HEngine::Initialize(HWND hwnd, int width, int height)
{
	DX::ThrowIfFailed(HRESULT(runtimeInit));				//런타임가동 체크

	//HWND hwnd = InitWindow(instance, width, height);		//윈도우 생성

	m_gamePad = std::make_unique<GamePad>();				//게임패드 

	m_keyboard = std::make_unique<Keyboard>();				//키보드

	m_mouse = std::make_unique<Mouse>();					//마우스
	m_mouse->SetWindow(hwnd);

	//해당객체에 윈도우 핸들과 클라이언트 크기를 셋해준다.
	m_pDeviceResources->SetWindow(hwnd, width, height);

	m_pDeviceResources->CreateDeviceResources();			//디바이스 등의 기본적인 자원들을 생성
	CreateDeviceDependentResources();						//생성한 디바이스를 통해서 다른 자원들을 생성

	//클라이언트 크기에 따라 변하는 자원들을 생성
	//해당 크기는 런타임 중에도 변할 수 있다. 그럴 경우 아래 함수를 다시 호출한다.
	m_pDeviceResources->CreateWindowSizeDependentResources();//스왑체인을 생성
	CreateWindowSizeDependentResources();					//Proj mat, view port 등 생성

	// Create DirectXTK for Audio objects
	AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
	eflags = eflags | AudioEngine_Debug;
#endif

	m_audEngine = std::make_unique<AudioEngine>(eflags);

	m_audioEvent = 0;
	m_audioTimerAcc = 10.f;
	m_retryDefault = false;

	wchar_t strFilePath[MAX_PATH];
	//DX::FindMediaFile(strFilePath, MAX_PATH, L"adpcmdroid.xwb");

	//m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), strFilePath);
	//DX::FindMediaFile(strFilePath, MAX_PATH, L"DADDY-DADDY-DO-feat-Airi-Suzuki- (online-audio-converter.com).wav");
	//m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), strFilePath);
	//m_effect1 = m_soundEffect->CreateInstance();
	//m_effect2 = m_waveBank->CreateInstance(10);

	//m_effect1->Play(true);
	//m_effect1->Pause();
	//m_effect2->Play();

	m_camera.SetPosition(0.0f, 0.0f, -50.0f);

}

#pragma region WindowCreate & Message Handlers 

/*
LRESULT CALLBACK
MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	static bool s_in_sizemove = false;
	static bool s_in_suspend = false;
	static bool s_minimized = false;
	static bool s_fullscreen = false;
	// Set s_fullscreen to true if defaulting to fullscreen.

	//InitWindow함수에서 저장해둔 포인터를 불러온다.
	auto sample = reinterpret_cast<HEngine*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
		if (!g_hNewAudio)
		{
			// Ask for notification of new audio devices
			DEV_BROADCAST_DEVICEINTERFACE filter = {};
			filter.dbcc_size = sizeof(filter);
			filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
			filter.dbcc_classguid = KSCATEGORY_AUDIO;

			g_hNewAudio = RegisterDeviceNotification(hWnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
		}
		break;

	case WM_CLOSE:
		if (g_hNewAudio)
		{
			UnregisterDeviceNotification(g_hNewAudio);
			g_hNewAudio = nullptr;
		}
		DestroyWindow(hWnd);
		break;

	case WM_DEVICECHANGE:
		switch (wParam)
		{
		case DBT_DEVICEARRIVAL:
		{
			auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
			if (pDev)
			{
				if (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
				{
					auto pInter = reinterpret_cast<const PDEV_BROADCAST_DEVICEINTERFACE>(pDev);
					if (pInter->dbcc_classguid == KSCATEGORY_AUDIO)
					{
						if (sample)
							sample->NewAudioDevice();
					}
				}
			}
		}
		break;

		case DBT_DEVICEREMOVECOMPLETE:
		{
			auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
			if (pDev)
			{
				if (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
				{
					auto pInter = reinterpret_cast<const PDEV_BROADCAST_DEVICEINTERFACE>(pDev);
					if (pInter->dbcc_classguid == KSCATEGORY_AUDIO)
					{
						if (sample)
							sample->NewAudioDevice();
					}
				}
			}
		}
		break;
		}
		return 0;

	case WM_PAINT:
		if (s_in_sizemove && sample)
		{
			sample->Tick();
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
				if (!s_in_suspend && sample)
					sample->OnSuspending();
				s_in_suspend = true;
			}
		}
		else if (s_minimized)
		{
			s_minimized = false;
			if (s_in_suspend && sample)
				sample->OnResuming();
			s_in_suspend = false;
		}
		else if (!s_in_sizemove && sample)
		{
			sample->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
		}
		break;

	case WM_ENTERSIZEMOVE:
		s_in_sizemove = true;
		break;

	case WM_EXITSIZEMOVE:
		s_in_sizemove = false;
		if (sample)
		{
			RECT rc;
			GetClientRect(hWnd, &rc);

			sample->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
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
		if (sample)
		{
			Keyboard::ProcessMessage(message, wParam, lParam);
			Mouse::ProcessMessage(message, wParam, lParam);

			if (wParam)
			{
				sample->OnActivated();
			}
			else
			{
				sample->OnDeactivated();
			}
		}
		break;

	case WM_POWERBROADCAST:
		switch (wParam)
		{
		case PBT_APMQUERYSUSPEND:
			if (!s_in_suspend && sample)
				sample->OnSuspending();
			s_in_suspend = true;
			return TRUE;

		case PBT_APMRESUMESUSPEND:
			if (!s_minimized)
			{
				if (s_in_suspend && sample)
					sample->OnResuming();
				s_in_suspend = false;
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_INPUT:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		Mouse::ProcessMessage(message, wParam, lParam);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		Keyboard::ProcessMessage(message, wParam, lParam);
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
				if (sample)
					sample->GetDefaultSize(width, height);

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
		Keyboard::ProcessMessage(message, wParam, lParam);
		break;

	case WM_MENUCHAR:
		// A menu is active and the user presses a key that does not correspond
		// to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
		return MAKELRESULT(0, MNC_CLOSE);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
*/

/*HWND HEngine::InitWindow(HINSTANCE instance, LONG clientWidth, LONG clientHeight)
{
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = MainWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = instance;
	wcex.hIcon = LoadIconW(instance, L"IDI_ICON");
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"random name";
	wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");

	RegisterClassExW(&wcex);

	RECT R = { 0, 0, clientWidth, clientHeight };

	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, FALSE);
	std::wstring caption = L"initialCaption";

	HWND hwnd = CreateWindowExW(0, L"random name", caption.c_str(), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, R.right - R.left, R.bottom - R.top, nullptr, nullptr, instance,
		nullptr);

	//아래 함수로 인해 WinProc함수에 들어가더라도 엔진 초기화 전에는 엔진의 기능을 사용하지 않는다.
	ShowWindow(hwnd, SW_SHOW);

	//엔진의 포인터를 윈도우에 세팅해준다. WndProc함수에서 이를 다시 불러와서 사용할 것이다.
	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(m_app));

	return hwnd;
}*/

void HEngine::OnActivated()
{

}

void HEngine::OnDeactivated()
{

}

void HEngine::OnSuspending()
{
	m_audEngine->Suspend();
}

void HEngine::OnResuming()
{
	m_timer.ResetElapsedTime();
	m_gamePadButtons.Reset();
	m_keyboardButtons.Reset();
	m_audEngine->Resume();
}

void HEngine::OnWindowSizeChanged(int width, int height)
{
	if (!m_pDeviceResources->WindowSizeChanged(width, height))
		return;

	CreateWindowSizeDependentResources();
}

void HEngine::NewAudioDevice()
{
	if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
	{
		// Setup a retry in 1 second
		m_audioTimerAcc = 1.f;
		m_retryDefault = true;
	}
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

	// Only update audio engine once per frame
	if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
	{
		// Setup a retry in 1 second
		m_audioTimerAcc = 1.f;
		m_retryDefault = true;
	}

	Render();
}

void HEngine::Update(DX::StepTimer const& timer)
{
	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

	//오디오 업데이트
	m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
	if (m_audioTimerAcc < 0)
	{
		if (m_retryDefault)
		{
			m_retryDefault = false;
			if (m_audEngine->Reset())
			{
				// Restart looping audio
				if (m_effect1.get() != nullptr)
					m_effect1->Play(true);
			}
		}
		else
		{
			m_audioTimerAcc = 4.f;

			if (m_waveBank.get() != nullptr)
				m_waveBank->Play(m_audioEvent++);

			if (m_audioEvent >= 11)
				m_audioEvent = 0;
		}
	}

	//게임패드 업데이트
	auto pad = m_gamePad->GetState(0);
	if (pad.IsConnected())
	{
		m_gamePadButtons.Update(pad);

		if (pad.IsViewPressed())
		{
			PostQuitMessage(0);
		}
	}
	else
	{
		m_gamePadButtons.Reset();
	}

	//키보드 업데이트
	auto kb = m_keyboard->GetState();
	m_keyboardButtons.Update(kb);

	if (kb.Escape)
	{
		PostQuitMessage(0);
	}

	//카메라 업데이트
	m_camera.UpdateViewMatrix();

	m_pModelManager->Update();

	m_passConstant.Update(m_camera, timer);

	m_graphicPipeline.Update();

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

	float aspectRatio = float(size.right) / float(size.bottom);
	float fovAngleY = 60.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	m_camera.SetLens(fovAngleY, size.right, size.bottom, 1.0f, 1200.f);


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
	case WM_CREATE:
		if (!g_hNewAudio)
		{
			// Ask for notification of new audio devices
			DEV_BROADCAST_DEVICEINTERFACE filter = {};
			filter.dbcc_size = sizeof(filter);
			filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
			filter.dbcc_classguid = KSCATEGORY_AUDIO;

			g_hNewAudio = RegisterDeviceNotification(hWnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
		}
		break;

	case WM_CLOSE:
		if (g_hNewAudio)
		{
			UnregisterDeviceNotification(g_hNewAudio);
			g_hNewAudio = nullptr;
		}
		DestroyWindow(hWnd);
		break;

	case WM_DEVICECHANGE:
		switch (wParam)
		{
		case DBT_DEVICEARRIVAL:
		{
			auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
			if (pDev)
			{
				if (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
				{
					auto pInter = reinterpret_cast<const PDEV_BROADCAST_DEVICEINTERFACE>(pDev);
					if (pInter->dbcc_classguid == KSCATEGORY_AUDIO)
					{
						if (m_app)
							m_app->NewAudioDevice();
					}
				}
			}
		}
		break;

		case DBT_DEVICEREMOVECOMPLETE:
		{
			auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
			if (pDev)
			{
				if (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
				{
					auto pInter = reinterpret_cast<const PDEV_BROADCAST_DEVICEINTERFACE>(pDev);
					if (pInter->dbcc_classguid == KSCATEGORY_AUDIO)
					{
						if (m_app)
							m_app->NewAudioDevice();
					}
				}
			}
		}
		break;
		}
		break;

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

	case WM_INPUT:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		Mouse::ProcessMessage(message, wParam, lParam);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		Keyboard::ProcessMessage(message, wParam, lParam);
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
		Keyboard::ProcessMessage(message, wParam, lParam);
		break;

		//case WM_MENUCHAR:
		//	// A menu is active and the user presses a key that does not correspond
		//	// to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
		//	return MAKELRESULT(0, MNC_CLOSE);
	}

	//return DefWindowProc(hWnd, message, wParam, lParam);
}

DirectX::Keyboard* HEngine::GetKeyboard()
{
	return m_keyboard.get();
}

DirectX::GamePad* HEngine::GetGamePad()
{
	return m_gamePad.get();
}

DirectX::Mouse* HEngine::GetMouse()
{
	return m_mouse.get();
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

HMaterialData* HEngine::CreateMaterial(const WCHAR* albedo, const WCHAR* roughness, const WCHAR* metallic, const WCHAR* ao, const WCHAR* normal, const WCHAR* height)
{
	return HTextureManager::GetInstance()->CreateMaterial(*m_resourceUploadBatch,
		albedo, roughness, metallic, ao, normal, height);
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

void HEngine::LoadSkyBox(const WCHAR* skyboxDDSFile)
{
	m_textureManager->LoadSkybox(*m_resourceUploadBatch, skyboxDDSFile);
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

RECT HEngine::GetClientRectFromEngine()
{
	return m_pDeviceResources->GetOutputSize();
}
#pragma endregion







