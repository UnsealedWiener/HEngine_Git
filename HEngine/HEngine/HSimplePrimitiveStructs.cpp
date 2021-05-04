#include "pch.h"
#include "HSimplePrimitiveStructs.h"
#include "HSimplePrimitiveManager.h"

void HProceduralGeometry_line_HEngine::Delete()
{
	managerController.DeleteMe();
}

void HProceduralGeometry_rect_HEngine::Delete()
{
	managerController.DeleteMe();
}

void HSimplePrimitive_HEngine::Delete()
{
	managerController.DeleteMe();
}
