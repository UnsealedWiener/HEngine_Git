#pragma once
#include"HUIManagerDefines.h"

namespace DX
{
	class DeviceResources;
	class StepTimer;
}
class HTextureManager;
class HModelManager;
using namespace DX;
using namespace Microsoft::WRL;

class HUIManager
{
	const DeviceResources* m_pDeviceResources;
	const HTextureManager* m_pTextureManager;
	const HModelManager* m_pModelManager;
	const StepTimer* m_pStepTimer;
	

	GraphicsMemory* m_pGraphicMemory;

	std::unordered_map<void*, std::unique_ptr<HUI>> m_uiList;


	ComPtr<ID3D12RootSignature>										m_pRootSignature;
	ComPtr<ID3D12PipelineState>										m_PSO;

	ComPtr<ID3D12Resource>											m_verticesResource;
	ComPtr<ID3D12Resource>											m_indicesResource;

	SharedGraphicsResource											m_structuredBuffer_perInstance;

	//font
	std::unique_ptr<DirectX::SpriteBatch>           m_sprites;
	std::unique_ptr<DirectX::SpriteFont>            m_font;
	std::unique_ptr<DirectX::DescriptorHeap>        m_FontDescriptorHeap;
	std::deque<DebugString>							m_debugStrings;

public:
	static HUIManager* GetInstance();
	void Initialize(GraphicsMemory* pGraphicMemory, DeviceResources* pDeviceResources, HTextureManager* pTextureManager,
		HModelManager* pModelManager, StepTimer* pStepTimer);
	void Draw();
	void Update();
	void CreateDeviceDependentResource(ResourceUploadBatch& resourceBatch);
	void CreateWindowSizeDependentResources();
	HUIData* CreateUI();
	void LoadFont(const WCHAR* spriteFontFile, ResourceUploadBatch& resourceBatch);
	void AddDebugString(DebugString debugString);

private:
	void CreateRootSignature_UI();
	void CreatePSO();
	void CreateVertexIndexBuffer();
	void CreateSpriteBatchForFont(ResourceUploadBatch& resourceBatch);
	
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();

	void Draw2DSprite();
	void DrawFont();

	HUIManager(){}

};
