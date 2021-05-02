//***************************************************************************************
// d3dUtil.h by Frank Luna (C) 2015 All Rights Reserved.
//
// General helper code.
//***************************************************************************************

#pragma once

#include <wrl.h>
#include <D3Dcompiler.h>
#include <string>

namespace d3dUtil
{
    Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

	Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	Microsoft::WRL::ComPtr<ID3DBlob> LoadCompiledShader(
		const BYTE* pCompiledShader); 

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
};

