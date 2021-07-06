#include "pch.h"
#include "HWaveEffect.h"
#include"DeviceResources.h"
#include"HPassConstant.h"
#include"d3dUtil.h"
#include"wave_VS.hlsl.h"
#include"wave_PS.hlsl.h"


HWaveEffect* HWaveEffect::GetInstance()
{
	static HWaveEffect waveEffect;
	return &waveEffect;
}

void HWaveEffect::Initialize(HPassConstant* pPassConstant)
{
	m_pPassConstant = pPassConstant;

	CreateRootSig();
	CreatePSO();
}

HWaveData* HWaveEffect::CreateWave(int m, int n, float dx, float dt, float speed, float damping)
{
	std::unique_ptr<Waves> pWave =
		std::make_unique<Waves>(m, n, dx, dt, speed, damping);

	void* pKey = reinterpret_cast<void*>(pWave.get());

	pWave->managerController = 
		HManagerController_map(this, &m_waveList, pKey);

	m_waveList[pKey] = std::move(pWave);

	return (HWaveData*)pKey;
}

void HWaveEffect::Update(float dTime)
{
	for (auto& wave : m_waveList)
	{
		Waves* pWave = wave.second.get();
		pWave->Update(dTime);
	}
}

void HWaveEffect::Draw()
{
	//이전 단계에서 렌더타겟, 깊이버퍼 등 설정이 되었다고 가정

	auto commandList =
	DX::DeviceResources::GetInstance()->GetCommandList();

	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetPipelineState(m_PSO_default.Get());
	commandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	commandList->SetGraphicsRootConstantBufferView((UINT)WaveRootParam::eSceneConstant,
		m_pPassConstant->GetPassConstantGpuAddress());
	for (auto& wave : m_waveList)
	{
		Waves* pWave = wave.second.get();
		D3D12_VERTEX_BUFFER_VIEW vbv = pWave->GetVertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = pWave->GetIndexBufferView();
		commandList->IASetVertexBuffers(0, 1, &vbv);
		commandList->IASetIndexBuffer(&ibv);

		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		commandList->SetGraphicsRootConstantBufferView((UINT)WaveRootParam::eWaveConstant,
			pWave->GetConstantBufferGpuAddress());

		commandList->DrawIndexedInstanced(pWave->GetIndexCnt(), 1, 0, 0, 0);
	}


}

void HWaveEffect::CreateRootSig()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[(UINT)WaveRootParam::eTotalCount];
	slotRootParameter[(UINT)WaveRootParam::eSceneConstant].InitAsConstantBufferView(0);
	slotRootParameter[(UINT)WaveRootParam::eWaveConstant].InitAsConstantBufferView(1);
	slotRootParameter[(UINT)WaveRootParam::eTexture].InitAsDescriptorTable(1, &texTable,
		D3D12_SHADER_VISIBILITY_PIXEL);
	
	auto staticSamplers = d3dUtil::GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)WaveRootParam::eTotalCount, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	DX::ThrowIfFailed(hr);

	DX::ThrowIfFailed(DX::DeviceResources::GetInstance()->GetD3DDevice()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.GetAddressOf())));

}

void HWaveEffect::CreatePSO()
{
	
	auto device = DX::DeviceResources::GetInstance()->GetD3DDevice();
	auto backBufferFormat = DX::DeviceResources::GetInstance()->GetBackBufferFormat();
	auto depthBufferFormat = DX::DeviceResources::GetInstance()->GetDepthBufferFormat();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc = {};

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	PsoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
	PsoDesc.pRootSignature = m_pRootSignature.Get();

	D3D12_SHADER_BYTECODE vs;
	vs.pShaderBytecode = g_pwave_VS;
	vs.BytecodeLength = sizeof(g_pwave_VS);
	D3D12_SHADER_BYTECODE ps;
	ps.pShaderBytecode = g_pwave_PS;
	ps.BytecodeLength = sizeof(g_pwave_PS);

	PsoDesc.VS = vs;
	PsoDesc.PS = ps;

	PsoDesc.RasterizerState =  CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	PsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

	PsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	PsoDesc.SampleMask = UINT_MAX;
	PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PsoDesc.NumRenderTargets = (UINT)4;
	PsoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	PsoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	PsoDesc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	PsoDesc.RTVFormats[3] = DXGI_FORMAT_R32G32B32A32_FLOAT;

	PsoDesc.SampleDesc.Count = 1;
	PsoDesc.SampleDesc.Quality = 0;

	PsoDesc.DSVFormat = depthBufferFormat;

	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&m_PSO_default)));
}
