#include"pch.h"
#include "HEngine.h"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{

	HEngine* engine = HEngine::GetApp();
	engine->Initialize(hInstance,1920,1080);

    MSG msg = {};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            engine->Tick();
        }
    }
    delete engine;
    CoUninitialize();
    return (int)msg.wParam;
}