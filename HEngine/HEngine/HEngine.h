#pragma once

#include"DeviceResources.h"
#include"StepTimer.h"
#include"Camera.h"
#include"EngineInterface.h"
#include"HTextureManager.h"
#include"HModelManager.h"
#include"HPassConstant.h"
#include"HGraphicPipeline.h"
#include"HPicking.h"
#include"HBufferManager.h"

class HEngine final : public DX::IDeviceNotify
{
private:
	HEngine() noexcept(false);
public:
	~HEngine();

	HEngine(HEngine&&) = default;
	HEngine& operator= (HEngine&&) = default;

	HEngine(HEngine const&) = delete;
	HEngine& operator= (HEngine const&) = delete;

	//singleton 싱글턴
	static HEngine* m_app;
	static HEngine* GetApp();

	// Initialization and management
	void Initialize(HWND hwnd, int width, int height);

	// Basic render loop
	void Tick();

	// IDeviceNotify 디바이스를 잃거나 다시 가져올 때 사용하는 함수
	virtual void OnDeviceLost() override;
	virtual void OnDeviceRestored() override;
	          
	// Messages
	void OnActivated();
	void OnDeactivated();
	void OnSuspending();
	void OnResuming();
	void OnWindowSizeChanged(int width, int height);
	void NewAudioDevice();

	// Properties
	void GetDefaultSize(int& width, int& height) const;

private:
	const DXGI_FORMAT c_backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	const DXGI_FORMAT c_depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
	const unsigned int c_backBufferCount = 2;
	D3D_FEATURE_LEVEL c_minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	DeviceResources*								m_pDeviceResources;
	HPassConstant									m_passConstant;
	HModelManager*									m_pModelManager;
	HGraphicPipeline								m_graphicPipeline;
	Camera											m_camera;
	HPicking										m_picking;
	HBufferManager*									m_pHBufferManager;
	DX::StepTimer									m_timer;

	void Update(DX::StepTimer const& timer);
	void Render();
	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	
	// Input devices.
	std::unique_ptr<DirectX::GamePad>				m_gamePad;
	std::unique_ptr<DirectX::Keyboard>				m_keyboard;
	std::unique_ptr<DirectX::Mouse>					m_mouse;
	DirectX::GamePad::ButtonStateTracker			m_gamePadButtons;
	DirectX::Keyboard::KeyboardStateTracker			m_keyboardButtons;
	DirectX::Mouse::ButtonStateTracker				m_mouseButtons;
	
	// DirectXTK objects.
	//그래픽메모리의 전반을 관리하는 객체(싱글턴)
	std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
	std::unique_ptr<DirectX::ResourceUploadBatch>	m_resourceUploadBatch;
	//오디오 엔진
	std::unique_ptr<DirectX::AudioEngine>           m_audEngine;
	std::unique_ptr<DirectX::WaveBank>              m_waveBank;
	std::unique_ptr<DirectX::SoundEffect>           m_soundEffect;
	std::unique_ptr<DirectX::SoundEffectInstance>   m_effect1;
	std::unique_ptr<DirectX::SoundEffectInstance>   m_effect2;
	
	uint32_t                                        m_audioEvent;
	float                                           m_audioTimerAcc;
	bool                                            m_retryDefault;


	HTextureManager* m_textureManager;
	
	//디버그용 문자열
	std::deque<DebugString> m_debugStrings;

	//엔진인터페이스용
	//함수들
	public:
		void StartSetting();
		void FinishSetting();
		float GetElapsedTime();
		void SetCamera(Camera camera);
		Camera* GetCamera();
		DirectX::Keyboard* GetKeyboard();
		DirectX::GamePad* GetGamePad();
		DirectX::Mouse* GetMouse();
		void AddDebugString(DebugString debugString);
		HInstanceData* Picking(unsigned int pickX, unsigned int pickY);
		void ProcessWndMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		HProceduralGeometry_line* CreateLine(unsigned char flag);
		HProceduralGeometry_rect* CreateRect(unsigned char flag);
		HSimplePrimitive* CreateBox(Vector3 Size, unsigned char flag);
		HMaterialData* CreateMaterial(const WCHAR* albedo, const WCHAR* roughness, const WCHAR* metallic,
			const WCHAR* ao, const WCHAR* normal, const WCHAR* height);
		HSpriteData* CreateSprite(const WCHAR* spriteFile);
		HModelData* CreateModelFromFbxFile(std::string fbxfile);
		HAnimData* CreateAnimationFromFbxFiles(std::vector<std::string> fbxNames);
		HModelData* CreateModelFromHModelFile(std::string fbxfile);
		HAnimData* CreateAnimationFromHAnimFiles(std::vector<std::string> fbxNames);
		HLightData* CreateLight(LightType lightType);
		HUIData* CreateUI();
		void LoadSkyBox(const WCHAR* skyboxDDSFile);
		void LoadFont(const WCHAR* spriteFontFile);
		void SetReflectionEffect(bool bOnOff);
		void SetShadowEffect(bool bOnOff);
		void SetSSAO(bool bOnOff);
		void SetWireFrame(bool bOnOff);
		RECT GetClientRectFromEngine();
};

