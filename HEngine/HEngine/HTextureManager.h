#pragma once
#include"EngineInterface.h"
#include"HManagerController.h"
#include<unordered_map>

#define MAX_MATERIAL_DESCRIPTORHEAP_SIZE 350
#define TEXTURE_COUNT_PER_MATERIAL 7
#define MAX_MATERIAL_COUNT MAX_MATERIAL_DESCRIPTORHEAP_SIZE/TEXTURE_COUNT_PER_MATERIAL

#define SKYBOXMAP_INDEX 300
#define SKYBOXMAP_COUNT 1

#define COLORCHIP_TEXTURECOUNT 4

#define MAX_SPRITE_DESCRIPTORHEAP_SIZE 200

struct HTextureManager;

struct HMaterial : public HMaterialData
{
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_albedo;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_roughness;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_metallic;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_ao;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_normal;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_height;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_emissive;


	//재질데이터가 삭제되서 인덱스가 바뀌면 상수 버퍼의 내용도 따라서 바뀌어야 한다.
	//따라서 상수버퍼 쪽에서 갱신된 데이터를 확인할 수 있도록 포인터로 변수로 두었다.
	std::shared_ptr<UINT>							m_index;

	HManagerController_map<HTextureManager, unordered_map<void*, std::unique_ptr<HMaterial>>> managerController;
	void Delete()override;
};

struct HSprite : public HSpriteData
{
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_sprite;

	//재질데이터가 삭제되서 인덱스가 바뀌면 상수 버퍼의 내용도 따라서 바뀌어야 한다.
	//따라서 상수버퍼 쪽에서 갱신된 데이터를 확인할 수 있도록 포인터로 변수로 두었다.
	std::shared_ptr<UINT>							m_index;

	HManagerController_map<HTextureManager, unordered_map<void*, std::unique_ptr<HSprite>>> managerController;
	void Delete()override;
};


struct HTextureManager
{
private:
	HTextureManager() {};

private:
	
	mutable bool m_bRasterizeDirty = false;
	mutable bool m_bRaytracingDirty = false;
	mutable bool m_bSpriteDirty = false;


	std::unique_ptr<DirectX::DescriptorHeap> m_materialDescriptorHeap;
	std::unique_ptr<DirectX::DescriptorHeap> m_skyboxDescriptorHeap;
	std::unique_ptr<DirectX::DescriptorHeap> m_colorChipDescriptorHeap;

	std::unique_ptr<DirectX::DescriptorHeap> m_spriteDescriptorHeap;

	std::unordered_map<void*, std::unique_ptr<HMaterial>> m_materialList;
	std::unordered_map<void*, std::unique_ptr<HSprite>> m_spriteList;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_skybox;

	//colorchip resources
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_baseColor;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_roughness;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_metallic;
	Microsoft::WRL::ComPtr<ID3D12Resource>			m_emissive;


	UINT m_materialOffset = 0;
	UINT m_spriteOffset = 0;

	

public:
	static HTextureManager* GetInstance();
	void Initialize(ID3D12Device* device);
	HMaterial* GetMaterial(void* matHandle);
	DirectX::DescriptorHeap* GetMaterialDescriptorHeap() const{ return m_materialDescriptorHeap.get(); }
	DirectX::DescriptorHeap* GetSkyboxDescriptorHeap() const { return m_skyboxDescriptorHeap.get(); }
	DirectX::DescriptorHeap* GetSpriteDescriptorHeap() const { return m_spriteDescriptorHeap.get(); }
	DirectX::DescriptorHeap* GetColorChipDescriptorHeap() const { return m_colorChipDescriptorHeap.get(); }


	void LoadSkybox(ResourceUploadBatch& batch, const WCHAR* skybox);
	void LoadColorChip(ResourceUploadBatch& batch, const WCHAR* baseColor,
		const WCHAR* roughness, const WCHAR* metallic, const WCHAR* emissive);

	bool GetRasterizeDirty()	const{return m_bRasterizeDirty;}
	bool GetRaytracingDirty()	const{return m_bRaytracingDirty;}
	void SetRasterizeDirty(bool dirty)  const{ m_bRasterizeDirty = dirty; }
	void SetRaytracingDirty(bool dirty) const{ m_bRaytracingDirty = dirty; }

	HMaterialData* CreateMaterial(ResourceUploadBatch& batch,
		const WCHAR* albedo, const WCHAR* roughness, const WCHAR* metallic,
		const WCHAR* ao, const WCHAR* normal, const WCHAR* height, const WCHAR* emissive);

	HSpriteData* CreateSprite(ResourceUploadBatch& batch, const WCHAR* sprite);


	void AfterDeleteProcess_material();
	void AfterDeleteProcess_sprite();

private:

	void CreateSRVs_material(ID3D12Device* device, HMaterial* material);
	void CreateSRV(ID3D12Device* device, ID3D12Resource* pResouce, DirectX::DescriptorHeap* pDescriptorHeap,
		UINT slot);
	void CreateNullDescriptor(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
};


