#pragma once
#include"HWave.h"

class HPassConstant;

enum class WaveRootParam
{
	eSceneConstant,
	eWaveConstant,
	eTexture,
	eTotalCount
};

class HWaveEffect
{
private:
	HWaveEffect(){}
private:
	const HPassConstant* m_pPassConstant;
private:
	std::unordered_map<void*, std::unique_ptr<Waves>> m_waveList;


public:
	static HWaveEffect* GetInstance();
	void Initialize(HPassConstant* pPassConstant);

	HWaveData* CreateWave(int m, int n, float dx, float dt, float speed, float damping);
	void Update(float dTime);
	void Draw();

	std::unordered_map<void*, std::unique_ptr<Waves>>& GetWaveList() { return m_waveList; }
private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_pRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO_default;

	void CreateRootSig();
	void CreatePSO();
};

