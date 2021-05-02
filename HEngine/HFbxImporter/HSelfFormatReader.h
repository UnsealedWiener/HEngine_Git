#pragma once
//#include"HFBXImporterFormat.h"
#include"HSelfFormatDefines.h"

class HSelfFormatReader
{
	HModelFormat* m_curruntHModel = nullptr;
	HAnimFormat* m_curruntHAnim = nullptr;
public:
	~HSelfFormatReader();
	HModelFormat* ReadHModelFile(const char* file);
	HAnimFormat* ReadHAnimFile(const char* file);

private:
	void DeleteCurruntReadHModel();
	void DeleteCurruntReadHAnim();
};

