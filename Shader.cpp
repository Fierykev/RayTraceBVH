#include <d3d12.h>
#include <fstream>
#include <d3dcompiler.h>
#include "Helper.h"
#include <wrl.h>

using namespace std;
using namespace Microsoft::WRL;

unsigned int ComputeCBufferSize(double size)
{
	double ratio = size / 16.0;

	return (unsigned int)ceil(ratio) * 16;
}

HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ComPtr<ID3DBlob> pErrorBlob;
	hr = D3DCompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		
		return hr;
	}

	return S_OK;
}

HRESULT ReadCSO(string filename, string& data)
{
	ifstream file(filename, ios::in | ios::binary);

	if (file.is_open())
		data.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	else
		return E_FAIL;

	return S_OK;
}