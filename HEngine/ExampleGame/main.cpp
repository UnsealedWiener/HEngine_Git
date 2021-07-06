#pragma comment(lib,"HEngine.lib")

#include"ExampleGame.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	ExampleGame game;

	game.Initialize(hInstance, 1920, 1080);

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
			game.Loop();
		}
	}

	game.Finalize();

	return (int)msg.wParam;
}