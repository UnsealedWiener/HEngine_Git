#include"pch.h"
#include"HRasterizeDefines.h"

using namespace HRasterizeDefines;

PSOTYPE HRasterizeDefines::operator*(PSOTYPE c)
{
	return c;
}

PSOTYPE HRasterizeDefines::operator++(PSOTYPE& x)
{
	return x = (PSOTYPE)(std::underlying_type<PSOTYPE>::type(x) + 1);
}

PSOTYPE HRasterizeDefines::begin(PSOTYPE r)
{
	return PSOTYPE::DEFAULT;
}

PSOTYPE HRasterizeDefines::end(PSOTYPE r)
{
	PSOTYPE l = PSOTYPE::WIREFRAME;
	return ++l;
}


