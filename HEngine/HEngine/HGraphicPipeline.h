#pragma once

#include"HRasterize.h"
#include"HRaytracing.h"
#include"HComputeLighting.h"
#include"HPostProcess.h"
#include"HSimplePrimitiveManager.h"
#include"HUIManager.h"
#include"HWaveEffect.h"

enum CurrentPipeline
{
	eRasterize,
	eRaytracing
};

enum class DescriptorList_NonShaderVisible_GraphicPipiline
{
	eReflection,
	eSSAO,
	TotalCount
};

namespace DX
{
	class StepTimer;
}

class HGraphicPipeline
{
private:

	DeviceResources* m_pDeviceResources;
	HModelManager* m_pModelManager;
	HRasterize m_rasterize;
	HRaytracing m_raytracing;
	HPostProcess* m_postProcess;
	HComputeLighting m_computeLighting;
	HUIManager* m_pHUIManager;
	HSimplePrimitiveManager* m_pSimplePrimitiveManager;
	HWaveEffect* m_pWaveEffect;

	//rasterize produced buffers(g-buffers, rendertarget, shaderResource)
	ComPtr<ID3D12Resource>			m_albedoBuffer;
	ComPtr<ID3D12Resource>			m_metallicRoughnessAoEmissiveBuffer;
	ComPtr<ID3D12Resource>			m_emissiveBuffer;

	ComPtr<ID3D12Resource>			m_normalBuffer;
	ComPtr<ID3D12Resource>			m_shadowBuffer;
	ComPtr<ID3D12Resource>			m_ssaoBuffer;

	//raytracing produced buffers(unordered access, shaderResource)
	ComPtr<ID3D12Resource>			m_reflectionBuffer;

	//compute shader produced buffers(unordered access, copy source)
	ComPtr<ID3D12Resource>			m_fianlResultBuffer;
	ComPtr<ID3D12Resource>			m_sceneBuffer;

	ComPtr<ID3D12Resource>			m_horizonBlurBuffer;
	ComPtr<ID3D12Resource>			m_verticalBlurBuffer;


	//ComPtr<ID3D12Resource>			m_resultBuffer_quaterSize;
	//ComPtr<ID3D12Resource>			m_finalresultBuffer;

	//else Buffer
	ComPtr<ID3D12Resource>			m_randomVectorBuffer; //used for ssao

	//CurrentPipeline m_currentPipeline = eRasterize;
	bool			m_bWireFrameOn = false;
	bool			m_bReflection = true;
	bool			m_bShadowOn = true;
	bool			m_bSSAOOn = true;
	bool			m_bDOFOn = false;


	bool			m_bRayracingSupported = false;

	
	std::unique_ptr<DescriptorHeap>		m_pDescriptorHeap_nonShaderVisible_forClear;


public:
	
	void Initialize(GraphicsMemory* pGraphicsMemory, DeviceResources* pDeviceResources, HModelManager* pModelManager,
		HPassConstant* pPassConstant, HTextureManager* pTextureManager, Camera* pCamera, StepTimer* pStepTimer);
	void Update(float dTime);
	void Draw();
	void CreateWindowSizeDependetResources();


	void CreateDeviceDependentResources(ResourceUploadBatch& resourceBatch);
	//void SetGraphicPipeline(CurrentPipeline pipeline);

	void SetWireFrame(bool onOff) {m_bWireFrameOn = onOff;};
	void SetReflection(bool onOff) { m_bReflection = onOff; };
	void SetShadow(bool onOff) { m_bShadowOn = onOff; };
	void SetSSAO(bool onOff) { m_bSSAOOn = onOff; };
	void SetDOF(bool onOff) { m_bDOFOn = onOff; }
	bool GetWireFrame() {return m_bWireFrameOn;}
	bool GetReflection() { return m_bReflection; }
	bool GetShadow() { return m_bShadowOn; }
	bool GetSSAO() { return m_bSSAOOn; }
	bool GetDOF() { return m_bDOFOn; }




private:
	void CreateGBuffers();

	void CreateDescriptorHeapForClear();
	void CreateDesriptorsForClear();
	void CreateRandomVectorBuffer(ResourceUploadBatch& resourceBatch);

};