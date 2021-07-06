#pragma once

#include<unordered_map>


class ExampleGame
{
private:
	ExampleGame(){}

private:
	//Devices from HEngine_DX12_3D
	HEngine_DX12_3D* m_p3DgraphicEngine;

	std::unique_ptr<Keyboard> m_keyboard;
	Keyboard::KeyboardStateTracker m_keyboardTracker;
	std::unique_ptr<Mouse> m_pMouse;
	Mouse::ButtonStateTracker m_mouseTracker;

	HInstanceData* m_pMyCharacter;


public:
	static ExampleGame* GetInstance();
	void Initialize(HINSTANCE hInstance, int clientWidth, int clientHeight);
	void Loop();
	void Finalize();

private:
	HWND InitWindow(HINSTANCE hInstance, int clientWidth, int clientHeight);
	void AddGrid();
	void OutputDebugString();
	void CameraControl(float dTime);

};

