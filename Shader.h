#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <Windows.h>
#include <string>

using namespace std;

unsigned int ComputeCBufferSize(double size);

HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

HRESULT ReadCSO(string filename, string& data);

#endif