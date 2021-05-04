#pragma once
#include"EngineInterface.h"


#include<unordered_map>


class ExampleGame
{
private:
	//Devices from HEngine_DX12_3D
	HEngine_DX12_3D* m_p3DgraphicEngine;

	Keyboard* m_keyboard;
	Keyboard::KeyboardStateTracker m_keyboardTracker;
	Mouse* m_pMouse;
	Mouse::ButtonStateTracker m_mouseTracker;


public:
	void Initialize(HINSTANCE hInstance, int clientWidth, int clientHeight);
	void Loop();
	void Finalize();

private:
	HWND InitWindow(HINSTANCE hInstance, int clientWidth, int clientHeight);
	void AddGrid();

};

