#include"pch.h"
#include"HBufferManager.h"

HBufferManager* HBufferManager::GetInstance()
{
	static HBufferManager bufferManager;
	return &bufferManager;
}

void HBufferManager::CreateDefaultHeapBuffer_device5(ID3D12Device5* pDevice, CD3DX12_RESOURCE_DESC resDesc, D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12Resource>& resource)
{
	CD3DX12_HEAP_PROPERTIES defaultHeapProp(D3D12_HEAP_TYPE_DEFAULT);

	HGraphicBuffer graphicBuffer;

	ComPtr<ID3D12Resource> m_resource;

	pDevice->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		resourceState,
		nullptr,
		IID_PPV_ARGS(&graphicBuffer.m_resource)
	);

	pDevice->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_GRAPHICS_PPV_ARGS(&graphicBuffer.m_fence));



	resource = graphicBuffer.m_resource;

	m_graphicBuffers.push_back(graphicBuffer);
}

void HBufferManager::CreateDefaultHeapBuffer_device0(ID3D12Device* pDevice, CD3DX12_RESOURCE_DESC resDesc, D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12Resource>& resource)
{
	CD3DX12_HEAP_PROPERTIES defaultHeapProp(D3D12_HEAP_TYPE_DEFAULT);

	HGraphicBuffer graphicBuffer;

	ComPtr<ID3D12Resource> m_resource;

	pDevice->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		resourceState,
		nullptr,
		IID_PPV_ARGS(&graphicBuffer.m_resource)
	);

	pDevice->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_GRAPHICS_PPV_ARGS(&graphicBuffer.m_fence));



	resource = graphicBuffer.m_resource;

	m_graphicBuffers.push_back(graphicBuffer);
}

void HBufferManager::Commit(ID3D12CommandQueue* pCommandQueue)
{
	for (int i = 0; i < m_graphicBuffers.size(); i++)
	{
		m_graphicBuffers[i].m_resource->AddRef();
		unsigned int refCnt = m_graphicBuffers[i].m_resource->Release();

		if (refCnt == 1)
		{
			if (m_graphicBuffers[i].fenceValue <= m_graphicBuffers[i].m_fence->GetCompletedValue())
			{
				m_graphicBuffers.erase(m_graphicBuffers.begin() + i);
			}
		}

		if (refCnt >= 2)
		{
			m_graphicBuffers[i].fenceValue++;
			pCommandQueue->Signal(m_graphicBuffers[i].m_fence.Get(), m_graphicBuffers[i].fenceValue);
		}
	}
}
