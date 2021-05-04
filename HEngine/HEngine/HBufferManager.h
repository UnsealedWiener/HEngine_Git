#pragma once

#include<vector>
//<���� ����>
//Map �Լ��� ���ؼ� �ǽð����� CPU���� ������ ������ UploadHeap�� ���۴�
//DirectXTK12 ���̺귯���� GraphicMemory���� ������ ���ش�. 
//������ ����Ʈ���̽��� BLAS�� DefaultHeap���� �����ؾ� �ϹǷ� GraphicMemory�� ���ؼ�
//������ ���� ���� ����. ���� BLAS�� �����ϰ� �����ϱ� ���� �� Ŭ������ �������.
using namespace Microsoft::WRL;

struct HGraphicBuffer
{
	ComPtr<ID3D12Fence> m_fence;
	UINT64				fenceValue = 0;
	ComPtr<ID3D12Resource> m_resource;
};

class HBufferManager
{
private:
	HBufferManager() {};
	~HBufferManager()
	{
		m_graphicBuffers.clear();
	}
public:
	static HBufferManager* GetInstance();

	std::vector<HGraphicBuffer> m_graphicBuffers;

	void CreateDefaultHeapBuffer_device5(ID3D12Device5* pDevice, CD3DX12_RESOURCE_DESC resDesc, D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12Resource>& resource);
	void CreateDefaultHeapBuffer_device0(ID3D12Device* pDevice, CD3DX12_RESOURCE_DESC resDesc, D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12Resource>& resource);

	void Commit(ID3D12CommandQueue* pCommandQueue);
};
