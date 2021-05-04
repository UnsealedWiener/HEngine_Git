#include"pch.h"
#include"HUIManagerDefines.h"
#include"HUIManager.h"
#include"HTextureManager.h"
void HUI::Delete()
{
	managerController.DeleteMe();
}

void HUI::SetSprite(HSpriteData* pSprite)
{
	HSprite* spritePointer =  (HSprite*)pSprite;

	pTextureIndex = spritePointer->m_index;

}
