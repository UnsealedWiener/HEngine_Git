#include "ExampleGame.h"
#include<DirectXColors.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HEngine_DX12_3D* p3DEngine =
		reinterpret_cast<HEngine_DX12_3D*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	if (p3DEngine != nullptr)
		p3DEngine->ProcessWndMessage(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_CREATE:
	case WM_CLOSE:
	case WM_DEVICECHANGE:
	case WM_PAINT:
	case WM_SIZE:
	case WM_ENTERSIZEMOVE:
	case WM_EXITSIZEMOVE:
	case WM_GETMINMAXINFO:
	case WM_ACTIVATEAPP:
	case WM_POWERBROADCAST:
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
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HWND ExampleGame::InitWindow(HINSTANCE hInstance, int clientWidth, int clientHeight)
{
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
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
		CW_USEDEFAULT, CW_USEDEFAULT, R.right - R.left, R.bottom - R.top, nullptr, nullptr, hInstance,
		nullptr);

	//아래 함수로 인해 WinProc함수에 들어가더라도 엔진 초기화 전에는 엔진의 기능을 사용하지 않는다.
	ShowWindow(hwnd, SW_SHOW);

	//엔진의 포인터를 윈도우에 세팅해준다. WndProc함수에서 이를 다시 불러와서 사용할 것이다.
	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&m_p3DgraphicEngine));

	return hwnd;
}

void ExampleGame::AddGrid()
{
	for (int i = 0; i < 101; i++)
	{
		HProceduralGeometry_line* line =
			m_p3DgraphicEngine->CreateLine(eNoOption_Primitive);


		if (i == 0)
		{
			line->color = XMFLOAT4(0, 0, 0, 1);
		}
		else
		{
			line->color = XMFLOAT4(0.0, 0.0, 1.0, 1);
		}
		line->dots[0] = XMFLOAT3(10 * i - 500, 0, -500);
		line->dots[1] = XMFLOAT3(10 * i - 500, 0, +500);

	}

	for (int i = 0; i < 101; i++)
	{

		HProceduralGeometry_line* line =
			m_p3DgraphicEngine->CreateLine(eNoOption_Primitive);

		if (i == 0)
		{
			line->color = XMFLOAT4(0, 0, 0, 1);
		}
		else
		{
			line->color = XMFLOAT4(0.3, 0.3, 0.3, 0.5);
		}
		line->dots[0] = XMFLOAT3(500, 0, 10 * i - 500);
		line->dots[1] = XMFLOAT3(-500, 0, 10 * i - 500);

	}
}

void ExampleGame::Initialize(HINSTANCE hInstance, int clientWidth, int clientHeight)
{
	HWND hWnd = InitWindow(hInstance, clientWidth, clientHeight);

	//graphic initialize
	m_p3DgraphicEngine = HEngine_DX12_3D::GetInstance();
	m_p3DgraphicEngine->InitEngine(hWnd, clientWidth, clientHeight);
	m_keyboard = m_p3DgraphicEngine->GetKeyboard();
	m_pMouse = m_p3DgraphicEngine->GetMouse();

	//그래픽 자원을 만들 때 StartSetting을 호출
	//그래픽 카드에 여러 명령을 한번에 보내주기 위함
	m_p3DgraphicEngine->StartSetting();

	//디폴트 스카이박스 세팅
	m_p3DgraphicEngine->LoadSkyBox(L"Media/Skybox/Skybox.dds");
	//디폴트 폰트 세팅
	m_p3DgraphicEngine->LoadFont(L"Media/Fonts/SegoeUI_18.spritefont");

	HModelData* pHModel_Crunch = m_p3DgraphicEngine->CreateModelFromHModelFile("Media/Model/Crunch/Crunch_LOD3.hmodel");
	HModelData* pHModel_Field = m_p3DgraphicEngine->CreateModelFromHModelFile("Media/Model/Field0/Field0.hmodel");

	std::vector<std::string> animList;
	animList.push_back("Media/Model/Crunch/Crunch@idle.hanim");
	animList.push_back("Media/Model/Crunch/Crunch@attack01.hanim");

	HAnimData* pHAnim = m_p3DgraphicEngine->CreateAnimationFromHAnimFiles(animList);

	HMaterialData* pHMat_Spaceshipt = m_p3DgraphicEngine->CreateMaterial(L"Media/Material/spaceShip/spaceShip_albedo.png",
		L"Media/Material/spaceShip/spaceShip_roughness.png",
		L"Media/Material/spaceShip/spaceShip_metallic.png",
		L"Media/Material/spaceShip/spaceShip_ao.png",
		L"Media/Material/spaceShip/spaceShip_normal.png",
		L"Media/Material/spaceShip/spaceShip_height.png");

	HMaterialData* pHMat_Bamboo = m_p3DgraphicEngine->CreateMaterial(L"Media/Material/bamboo/bamboo_albedo.png",
		L"Media/Material/bamboo/bamboo_roughness.png",
		L"Media/Material/bamboo/bamboo_metallic.png",
		L"Media/Material/bamboo/bamboo_ao.png",
		L"Media/Material/bamboo/bamboo_normal.png",
		nullptr);

	//그래픽 자원을 생성하는 명령을 그래픽 카드에 보내줌
	//이후 모든 명령이 수행될 때까지 기다린다.
	m_p3DgraphicEngine->FinishSetting();

	pHModel_Crunch->SetAnimation(pHAnim);

	m_p3DgraphicEngine->GetCamera()->LookAt(Vector3(0, 500, -500), Vector3(0, 0, 0), Vector3(0, 1, 0));

	m_pMyCharacter = pHModel_Crunch->AddInstance(eNoOption_EI);
	m_pMyCharacter->SetMaterial(pHMat_Spaceshipt);
	m_pMyCharacter->animationName = "idle";

	pHModel_Field->AddInstance(eNoOption_EI)->SetMaterial(pHMat_Bamboo);

	HLightData* pLightData =  m_p3DgraphicEngine->CreateLight(LightType::eDirectLight);
	pLightData->strength = Vector3(0.7f, 0.7f, 0.7f);

	AddGrid();

}

void ExampleGame::Loop()
{
	float dTime = m_p3DgraphicEngine->GetElapsedTime();

	m_pMyCharacter->animationTime += dTime;

	m_keyboardTracker.Update(m_keyboard->GetState());
	m_mouseTracker.Update(m_pMouse->GetState());

	Camera* camera =  m_p3DgraphicEngine->GetCamera();

	//free camera
	{
		if (m_keyboardTracker.GetLastState().W)
			camera->Walk(50.0f * dTime);
		if (m_keyboardTracker.GetLastState().S)
			camera->Walk(-50.0f * dTime);
		if (m_keyboardTracker.GetLastState().A)
			camera->Strafe(-50.0f * dTime);
		if (m_keyboardTracker.GetLastState().D)
			camera->Strafe(+50.0f * dTime);

		static int lastPosX = 0;
		static int lastPosY = 0;

		if (m_mouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED)
		{
			lastPosX = m_mouseTracker.GetLastState().x;
			lastPosY = m_mouseTracker.GetLastState().y;
		}
		if (m_mouseTracker.leftButton == Mouse::ButtonStateTracker::HELD)
		{
			int xVec = m_mouseTracker.GetLastState().x - lastPosX;
			int yVec = m_mouseTracker.GetLastState().y - lastPosY;
			lastPosX = m_mouseTracker.GetLastState().x;
			lastPosY = m_mouseTracker.GetLastState().y;

			float dx = XMConvertToRadians(0.25f * static_cast<float>(xVec));
			float dy = XMConvertToRadians(0.25f * static_cast<float>(yVec));

			camera->Pitch(dy);
			camera->RotateY(dx);
		}

	}

	DebugString debugString;
	debugString.color = Color(0.5f, 0.5f, 0.5f);
	debugString.message = "MoveCamera : W/A/S/D & Mouse Left Click";
	debugString.posX = 100;
	debugString.posY = 100;


	m_p3DgraphicEngine->AddDebugString(debugString);

	m_p3DgraphicEngine->Loop();
}

void ExampleGame::Finalize()
{
	m_p3DgraphicEngine->EndEngine();
}
