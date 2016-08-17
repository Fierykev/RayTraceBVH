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

	~Image();

	bool loadImage(const wchar_t* filename);

	void createTexture(ComPtr<ID3D12Device> device);

	void uploadTexture(ID3D12GraphicsCommandList* commandList);

	ILubyte* getData();

private:

	ComPtr<ID3D12Resource> texture2D, uploadText;
	D3D12_SUBRESOURCE_DATA subData;

	int imageID = -1;
	int width;
	int height;
	int format;

	ILubyte* data;
};

#endif