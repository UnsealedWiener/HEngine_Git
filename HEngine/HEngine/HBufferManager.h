#pragma once

#include<vector>
//<설계 개요>
//Map 함수를 통해서 실시간으로 CPU에서 수정이 가능한 UploadHeap의 버퍼는
//DirectXTK12 라이브러리의 GraphicMemory에서 관리를 해준다. 
//하지만 레이트레이싱의 BLAS는 DefaultHeap으로 생성해야 하므로 GraphicMemory를 통해서
//관리를 해줄 수가 없다. 따라서 BLAS를 간편하게 관리하기 위해 이 클래스를 만들었다.
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
