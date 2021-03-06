#include "pch.h"
#include "HModelManagerStructs.h"
#include "HModelManager.h"
#include "DeviceResources.h"
#include "HTextureManager.h"

void HModel::Delete()
{
	managerController.DeleteMe();
}

HInstanceData* HModel::AddInstance(ShaderType type)
{
	std::shared_ptr<HInstance> pInstance
		= std::make_shared<HInstance>();

	PSOTYPE tempType;

	switch (type)
	{
	case ShaderType::DEFAULT:
		tempType = PSOTYPE::DEFAULT;
		break;
	case ShaderType::COLORCHIP:
		tempType = PSOTYPE::COLORCHIP;
		break;
	case ShaderType::WIREFRAME:
		tempType = PSOTYPE::WIREFRAME;
		break;
	default:
		break;
	}

	pInstance->m_psoType = tempType;

	pInstance->pModel = HModelManager::GetInstance()->m_models[this];
	HModelManager::GetInstance()->m_instances[pInstance.get()] = pInstance;

	HInstanceData* ptr = pInstance.get();

	pInstance->managerController = HManagerController_map(this, &instances[tempType], ptr);

	instances[tempType][ptr] = move(pInstance);

	return ptr;
}

void HModel::SetAnimation(HAnimData* pAnimData)
{
	HAnim* pAnim = (HAnim*)pAnimData;

	for (auto& e : pAnim->rawData.animBundle)
	{
		if (e.second->AxisSystempModify != rawData.AxisSystempModify)
		{
			HerrorMessage(L"The model axisSystem and animation axisSystem don't match.")
				throw;
		}

		std::vector<HBoneAnim>& boneAnim = e.second->boneAnim;

		for (int i = 0; i < rawData.pAllBoneNodes.size(); i++)
		{
			if (rawData.pAllBoneNodes[i]->name != boneAnim[i].name)
			{
				HerrorMessage(L"The model bones and animation bones don't match.")
					throw;
			}
		}
	}

	m_pAnim = pAnim->managerController.GetContainer()->operator[](pAnim);
}

HAnimData* HModel::GetAnimation()
{
	return m_pAnim.get();
}

void HInstance::Delete()
{
	managerController.DeleteMe();
}

void HInstance::SetMaterial(HMaterialData* pMaterial, unsigned int slot)
{
	if(slot != 0 && slot != 1)
		HerrorMessage(L"The slot has to be 0 or 1.")

	HMaterial* matPointer = (HMaterial*)pMaterial;

	if (slot == 0)
		pMat = matPointer->m_index;
	if (slot == 1)
		pMatSecond = matPointer->m_index;
}

void HInstance::SetShaderFlag(ShaderType type)
{

	PSOTYPE tempType;

	switch (type)
	{
	case ShaderType::DEFAULT:
		tempType = PSOTYPE::DEFAULT;
		break;
	case ShaderType::COLORCHIP:
		tempType = PSOTYPE::COLORCHIP;
		break;
	case ShaderType::WIREFRAME:
		tempType = PSOTYPE::WIREFRAME;
		break;
	default:
		break;
	}

	if (m_psoType == tempType)
		return;

	HModel* pModelPointer = pModel.lock().get();
	pModelPointer->instances[tempType][this].swap(pModelPointer->instances[m_psoType][this]);
	managerController = HManagerController_map(pModelPointer, &pModelPointer->instances[tempType], this);
	pModelPointer->instances[m_psoType].erase(this);
	m_psoType = tempType;
}

Matrix HInstance::GetBoneTMAtCurruntFrame(std::string boneName)
{
	return boneTMCurrFrame[boneName] *pModel.lock().get()->rawData.AxisSystempModify;
}

void HAnim::Delete()
{
	managerController.DeleteMe();
}
