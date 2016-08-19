#ifndef IMAGE_H
#define IMAGE_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <IL/il.h>

#include <iostream>

using namespace Microsoft::WRL;
using namespace std;

class Image
{
public:

	Image();

	void deleteImage();

	bool loadImage(ID3D12Device* device, const wchar_t* filename);

	void uploadTexture(ID3D12GraphicsCommandList* commandList);

	ILubyte* getData();

private:

	void createTexture(ID3D12Device* device);

	ComPtr<ID3D12Resource> texture2D, uploadText;

	int width;
	int height;

	unsigned char* data = nullptr;
};

#endif