#include"pch.h"
#include"HSimplePrimitiveManager.h"
#include"HPassConstant.h"
#include"DeviceResources.h"

HSimplePrimitiveManager* HSimplePrimitiveManager::GetInstance()
{
	static HSimplePrimitiveManager manager;

	return &manager;
}

void HSimplePrimitiveManager::Initialize(DeviceResources* pDeviceResources, HPassConstant* pPassConstant, Camera* pCamera)
{
	m_pDeviceResources = pDeviceResources;
	m_pPassConstant = pPassConstant;
	m_pCamera = pCamera;
}

void HSimplePrimitiveManager::CreateDeviceDependentResources()
{
	auto device = m_pDeviceResources->GetD3DDevice();
	m_primitivebatch = make_unique<PrimitiveBatch<VertexPositionColor>>(device);

	RenderTargetState rtState(m_pDeviceResources->GetBackBufferFormat(),
		m_pDeviceResources->GetDepthBufferFormat());

	//line base effect
	{
		EffectPipelineStateDescription pd(
			&VertexPositionColor::InputLayout,
			CommonStates::AlphaBlend,
			CommonStates::DepthDefault,
			CommonStates::CullNone,
			rtState,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

		pd.renderTargetState.sampleDesc.Count = 1;
		pd.renderTargetState.sampleDesc.Quality = 0;

		m_lineEffect[eNoOption_Primitive] = make_unique<BasicEffect>(device,
			EffectFlags::VertexColor, pd);

		pd.depthStencilDesc.DepthEnable = FALSE;

		m_lineEffect[eDepthOff_Primitive] = make_unique<BasicEffect>(device,
			EffectFlags::VertexColor, pd);
	}
	//rect
	for (unsigned char flag = 0; flag <= eAllOption_Primitive; flag++)
	{
		EffectPipelineStateDescription pd(
			&VertexPositionColor::InputLayout,
			CommonStates::AlphaBlend,
			((flag & eDepthOff_Primitive) != 0) ? CommonStates::DepthNone : CommonStates::DepthDefault,
			((flag & eWireFrame_Primitive) != 0) ? CommonStates::Wireframe : CommonStates::CullNone,
			rtState,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

		pd.renderTargetState.sampleDesc.Count = 1;
		pd.renderTargetState.sampleDesc.Quality = 0;

		m_rectEffect[flag] = make_unique<BasicEffect>(device,
			EffectFlags::VertexColor, pd);
	}

	//primitive base effect
	for (unsigned char flag = 0; flag <= eAllOption_Primitive; flag++)
	{
		EffectPipelineStateDescription pd(
			&GeometricPrimitive::VertexType::InputLayout,
			CommonStates::AlphaBlend,
			((flag & eDepthOff_Primitive) != 0) ? CommonStates::DepthNone :CommonStates::DepthDefault,
			((flag & eWireFrame_Primitive) != 0) ? CommonStates::Wireframe : CommonStates::CullNone,
			rtState,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

		pd.renderTargetState.sampleDesc.Count = 1;
		pd.renderTargetState.sampleDesc.Quality = 0;

		m_primitiveEffect[flag] = make_unique<BasicEffect>(device,
			EffectFlags::PerPixelLighting, pd);

		if (flag & eWireFrame_Primitive == 0)
		{
			m_primitiveEffect[flag]->EnableDefaultLighting();
			m_primitiveEffect[flag]->SetLightDiffuseColor(0, XMVectorSet(1.f, 1.f, 1.f, 1));
			m_primitiveEffect[flag]->SetLightDirection(0, XMVectorSet(0.57735f, -0.57735f, 0.57735f, 0));
		}
	}
}

void HSimplePrimitiveManager::CreateWindowSizeDependentResources()
{
	for (pair<const unsigned char, unique_ptr<BasicEffect>>& lineEffect : m_lineEffect)
	{
		lineEffect.second->SetProjection(m_pCamera->GetProj());
	}

	for (pair<const unsigned char, unique_ptr<BasicEffect>>& rectEffect : m_rectEffect)
	{
		rectEffect.second->SetProjection(m_pCamera->GetProj());
	}

	for (pair<const unsigned char, unique_ptr<BasicEffect>>& triangleEffect : m_primitiveEffect)
	{
		triangleEffect.second->SetProjection(m_pCamera->GetProj());
	}
}

void HSimplePrimitiveManager::Update()
{
	for (pair<const unsigned char, unique_ptr<BasicEffect>>& lineEffect : m_lineEffect)
	{
		lineEffect.second->SetView(m_pCamera->GetView());
		lineEffect.second->SetProjection(m_pCamera->GetProj());
	}

	for (pair<const unsigned char, unique_ptr<BasicEffect>>& rectEffect : m_rectEffect)
	{
		rectEffect.second->SetView(m_pCamera->GetView());
		rectEffect.second->SetProjection(m_pCamera->GetProj());
	}

	for (pair<const unsigned char, unique_ptr<BasicEffect>>& triangleEffect : m_primitiveEffect)
	{
		triangleEffect.second->SetView(m_pCamera->GetView());
		triangleEffect.second->SetProjection(m_pCamera->GetProj());
	}
}

void HSimplePrimitiveManager::Draw(ID3D12GraphicsCommandList* commandList)
{
	auto rtv = m_pDeviceResources->GetRenderTargetView();
	auto dsv = m_pDeviceResources->GetDepthStencilView();
	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	m_primitivebatch->Begin(commandList);

	for (pair<const unsigned char, unordered_map<void*, unique_ptr<HProceduralGeometry_line_HEngine>>>& lines : m_lines)
	{
		if (lines.second.size() == 0)
			continue;

		m_lineEffect[lines.first]->Apply(commandList);

		for (pair<void* const, unique_ptr<HProceduralGeometry_line_HEngine>>& line : lines.second)
		{
			m_primitivebatch->DrawLine(
				VertexPositionColor(XMVECTOR(line.second->dots[0]), XMVECTOR(line.second->color)),
				VertexPositionColor(XMVECTOR(line.second->dots[1]), XMVECTOR(line.second->color)));
		}
	}

	m_primitivebatch->End();

	m_primitivebatch->Begin(commandList);
	for (pair<const unsigned char, unordered_map<void*, unique_ptr<HProceduralGeometry_rect_HEngine>>>& rects : m_rects)
	{
		if (rects.second.size() == 0)
			continue;

		m_rectEffect[rects.first]->Apply(commandList);

		for (pair<void* const, unique_ptr<HProceduralGeometry_rect_HEngine>>& rect : rects.second)
		{
			m_primitivebatch->DrawQuad(
				VertexPositionColor(XMVECTOR(rect.second->dots[0]), XMVECTOR(rect.second->color)),
				VertexPositionColor(XMVECTOR(rect.second->dots[1]), XMVECTOR(rect.second->color)),
				VertexPositionColor(XMVECTOR(rect.second->dots[2]), XMVECTOR(rect.second->color)), 
				VertexPositionColor(XMVECTOR(rect.second->dots[3]), XMVECTOR(rect.second->color)));
		}
	}

	m_primitivebatch->End();

	for (pair<const unsigned char, unordered_map<void* ,unique_ptr<HSimplePrimitive_HEngine>>>& primitives : m_simplePrimitives)
	{
		if (primitives.second.size() == 0)
			continue;

		for (pair<void* const, unique_ptr<HSimplePrimitive_HEngine>>& primitive : primitives.second)
		{
			m_primitiveEffect[primitives.first]->SetWorld(primitive.second->worldTM);
			m_primitiveEffect[primitives.first]->Apply(commandList);
			primitive.second->geometryPrimitive->Draw(commandList);
			
		}
	}
}

HProceduralGeometry_line* HSimplePrimitiveManager::CreateLine(unsigned char flag)
{

	if((flag & eWireFrame_Primitive) != 0)
		HerrorMessage(L"line can't have wireFrame flag")

	unique_ptr<HProceduralGeometry_line_HEngine> pLine = make_unique<HProceduralGeometry_line_HEngine>();

	HProceduralGeometry_line* ptr = pLine.get();

	pLine->managerController = HManagerController(this, &m_lines[flag], ptr);

	m_lines[flag][ptr] = move(pLine);

	return ptr;
}

HProceduralGeometry_rect* HSimplePrimitiveManager::CreateRect(unsigned char flag)
{
	unique_ptr<HProceduralGeometry_rect_HEngine> pRect = make_unique<HProceduralGeometry_rect_HEngine>();

	HProceduralGeometry_rect* ptr = pRect.get();

	pRect->managerController = HManagerController(this, &m_rects[flag], ptr);

	m_rects[flag][ptr] = move(pRect);

	return ptr;
}

HSimplePrimitive* HSimplePrimitiveManager::CreateBox(Vector3 Size, unsigned char flag)
{
	unique_ptr<HSimplePrimitive_HEngine> pBox = make_unique<HSimplePrimitive_HEngine>();

	std::unique_ptr<GeometricPrimitive> geometryPrimitive =
		 GeometricPrimitive::CreateBox(Size, false);

	pBox->geometryPrimitive = GeometricPrimitive::CreateBox(Size, false);

	HSimplePrimitive* ptr = pBox.get();

	pBox->managerController = HManagerController(this, &m_simplePrimitives[flag], ptr);

	m_simplePrimitives[flag][ptr] = move(pBox);

	return ptr;
}
