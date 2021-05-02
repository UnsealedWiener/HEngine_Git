#include"pch.h"
#include"HTextureManager.h"
#include"d3dUtil.h"
#include"DeviceResources.h"

HTextureManager* HTextureManager::GetInstance()
{
	static HTextureManager textureManager;
	return &textureManager;
}

void HTextureManager::Initialize(ID3D12Device* device)
{
	m_materialDescriptorHeap = std::make_unique<DirectX::DescriptorHeap>(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		MAX_MATERIAL_DESCRIPTORHEAP_SIZE);
	m_skyboxDescriptorHeap = std::make_unique<DirectX::DescriptorHeap>(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		SKYBOXMAP_COUNT);
	m_spriteDescriptorHeap = std::make_unique<DirectX::DescriptorHeap>(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		MAX_SPRITE_DESCRIPTORHEAP_SIZE);
}

HMaterial* HTextureManager::GetMaterial(void* matHandle)
{
	return m_materialList[matHandle].get();
}

void HTextureManager::LoadSkybox(ID3D12Device* device, ResourceUploadBatch& batch, const WCHAR* skybox)
{
	DirectX::CreateDDSTextureFromFile(device,
		batch, skybox, m_skybox.ReleaseAndGetAddressOf());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_skybox->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_skybox->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	device->CreateShaderResourceView(m_skybox.Get(), &srvDesc, m_skyboxDescriptorHeap->GetCpuHandle(0));
}

HMaterialData* HTextureManager::CreateMaterial(ResourceUploadBatch& batch, const WCHAR* albedo, const WCHAR* roughness, const WCHAR* metallic, const WCHAR* ao, const WCHAR* normal, const WCHAR* height)
{
	auto device = DX::DeviceResources::GetInstance()->GetD3DDevice();

	std::unique_ptr<HMaterial> hMaterial = std::make_unique<HMaterial>();
	hMaterial->m_index = std::make_shared<UINT>(m_materialOffset);

	HRESULT hr;

	if (albedo != nullptr)
	{
		hr = DirectX::CreateDDSTextureFromFile(device, batch, albedo, hMaterial->m_albedo.ReleaseAndGetAddressOf(), true);
		if (hr == E_FAIL)
			DirectX::CreateWICTextureFromFile(device, batch, albedo, hMaterial->m_albedo.ReleaseAndGetAddressOf(), true);
	}
	if (roughness != nullptr)
	{
		hr = DirectX::CreateDDSTextureFromFile(device, batch, roughness, hMaterial->m_roughness.ReleaseAndGetAddressOf(), true);
		if (hr == E_FAIL)
			DirectX::CreateWICTextureFromFile(device, batch, roughness, hMaterial->m_roughness.ReleaseAndGetAddressOf(), true);
	}
	if (metallic != nullptr)
	{
		hr = DirectX::CreateDDSTextureFromFile(device, batch, metallic, hMaterial->m_metallic.ReleaseAndGetAddressOf(), true);
		if (hr == E_FAIL)
			DirectX::CreateWICTextureFromFile(device, batch, metallic, hMaterial->m_metallic.ReleaseAndGetAddressOf(), true);
	}
	if (ao != nullptr)
	{
		hr = DirectX::CreateDDSTextureFromFile(device, batch, ao, hMaterial->m_ao.ReleaseAndGetAddressOf(), true);
		if (hr == E_FAIL)
			DirectX::CreateWICTextureFromFile(device, batch, ao, hMaterial->m_ao.ReleaseAndGetAddressOf(), true);
	}
	if (normal != nullptr)
	{
		hr = DirectX::CreateDDSTextureFromFile(device, batch, normal, hMaterial->m_normal.ReleaseAndGetAddressOf(), true);
		if (hr == E_FAIL)
			DirectX::CreateWICTextureFromFile(device, batch, normal, hMaterial->m_normal.ReleaseAndGetAddressOf(), true);
	}
	if (height != nullptr)
	{
		hr = DirectX::CreateDDSTextureFromFile(device, batch, height, hMaterial->m_height.ReleaseAndGetAddressOf(), true);
		if (hr == E_FAIL)
			DirectX::CreateWICTextureFromFile(device, batch, height, hMaterial->m_height.ReleaseAndGetAddressOf(), true);
	}

	CreateSRVs_material(device, hMaterial.get());


	HMaterialData* ptr = reinterpret_cast<HMaterialData*>(hMaterial.get());

	hMaterial->managerController = HManagerController(this, &m_materialList, ptr);
	hMaterial->managerController.SetAfterDeleteProcess(&HTextureManager::AfterDeleteProcess_material);

	m_materialList[ptr] = std::move(hMaterial);

	m_bRasterizeDirty = true;
	m_bRaytracingDirty = true;

	return ptr;
}

HSpriteData* HTextureManager::CreateSprite(ResourceUploadBatch& batch, const WCHAR* spriteFile)
{
	auto device = DX::DeviceResources::GetInstance()->GetD3DDevice();

	std::unique_ptr<HSprite> hSprite = std::make_unique<HSprite>();
	hSprite->m_index = std::make_shared<UINT>(m_spriteOffset);

	HRESULT hr;

	if (spriteFile != nullptr)
	{
		hr = DirectX::CreateDDSTextureFromFile(device, batch, spriteFile, hSprite->m_sprite.ReleaseAndGetAddressOf(), true);
		if (hr == E_FAIL)
			DirectX::CreateWICTextureFromFile(device, batch, spriteFile, hSprite->m_sprite.ReleaseAndGetAddressOf(), true);
	}

	CreateSRV(device, hSprite->m_sprite.Get(), m_spriteDescriptorHeap.get(), m_spriteOffset++);

	HSpriteData* ptr = reinterpret_cast<HSpriteData*>(hSprite.get());

	hSprite->managerController = HManagerController(this, &m_spriteList, ptr);
	hSprite->managerController.SetAfterDeleteProcess(&HTextureManager::AfterDeleteProcess_sprite);

	m_spriteList[ptr] = std::move(hSprite);

	m_bSpriteDirty = true;

	return ptr;
}

void HTextureManager::AfterDeleteProcess_material()
{
	auto device = DX::DeviceResources::GetInstance()->GetD3DDevice();

	m_materialOffset = 0;

	for (auto& e : m_materialList)
	{
		if (e.second.get() != nullptr)
			CreateSRVs_material(device, e.second.get());
	}


	m_bRasterizeDirty = true;
	m_bRaytracingDirty = true;
}

void HTextureManager::AfterDeleteProcess_sprite()
{
	auto device = DX::DeviceResources::GetInstance()->GetD3DDevice();

	m_spriteOffset = 0;

	for (auto& e : m_spriteList)
	{
		if (e.second.get() != nullptr)
			CreateSRV(device, e.second->m_sprite.Get(), m_spriteDescriptorHeap.get(), m_spriteOffset++);
	}

	m_bSpriteDirty = true;

}

void HTextureManager::CreateSRVs_material(ID3D12Device* device, HMaterial* material)
{
	*material->m_index = m_materialOffset;
	
	if (material->m_albedo == nullptr)
		CreateNullDescriptor(device, m_materialOffset++);
	else
		CreateSRV(device, material->m_albedo.Get(), m_materialDescriptorHeap.get(), m_materialOffset++);

	if (material->m_roughness == nullptr)
		CreateNullDescriptor(device, m_materialOffset++);
	else
		CreateSRV(device, material->m_roughness.Get(), m_materialDescriptorHeap.get(), m_materialOffset++);

	if (material->m_metallic == nullptr)
		CreateNullDescriptor(device, m_materialOffset++);
	else
		CreateSRV(device, material->m_metallic.Get(), m_materialDescriptorHeap.get(), m_materialOffset++);

	if (material->m_ao == nullptr)
		CreateNullDescriptor(device, m_materialOffset++);
	else
		CreateSRV(device, material->m_ao.Get(), m_materialDescriptorHeap.get(), m_materialOffset++);

	if (material->m_normal == nullptr)
		CreateNullDescriptor(device, m_materialOffset++);
	else
		CreateSRV(device, material->m_normal.Get(), m_materialDescriptorHeap.get(), m_materialOffset++);

	if (material->m_height == nullptr)
		CreateNullDescriptor(device, m_materialOffset++);
	else
		CreateSRV(device, material->m_height.Get(), m_materialDescriptorHeap.get(), m_materialOffset++);
}

void HTextureManager::CreateSRV(ID3D12Device* device, ID3D12Resource* pResouce,
	DirectX::DescriptorHeap* pDescriptorHeap, UINT slot)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = pResouce->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = pResouce->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	device->CreateShaderResourceView(pResouce, &srvDesc, pDescriptorHeap->GetCpuHandle(slot));
}

void HTextureManager::CreateNullDescriptor(ID3D12Device* device, UINT slot)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	
	device->CreateShaderResourceView(nullptr, &srvDesc, m_materialDescriptorHeap->GetCpuHandle(slot));
}

void HMaterial::Delete()
{
	DX::DeviceResources::GetInstance()->WaitForGpu();
	managerController.DeleteMe();
}

void HSprite::Delete()
{
	DX::DeviceResources::GetInstance()->WaitForGpu();
	managerController.DeleteMe();
}
