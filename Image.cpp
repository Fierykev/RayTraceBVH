#include "d3dx12.h"
#include "Image.h"

bool devILInit = false;

Image::Image()
{
	if (!devILInit)
		ilInit();

	// init devIL
	devILInit = true;
}

void Image::deleteImage()
{
	// delete the data
	if (data)
	{
		delete[] data;
		
		// TODO: cleanup d3d
	}
}

bool Image::loadImage(ID3D12Device* device, const wchar_t* filename)
{
	ILuint imageID = -1;

	ilGenImages(1, &imageID);
	ilBindImage(imageID);

	if (!ilLoadImage(filename))
		return false;

	width = ilGetInteger(IL_IMAGE_WIDTH);
	height = ilGetInteger(IL_IMAGE_HEIGHT);

	// convert the image to a usable format
	data = new unsigned char[width * height * 4];
	ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA,
		IL_UNSIGNED_BYTE, data);

	// unbind the image and delete it
	ilBindImage(0);
	ilDeleteImage(imageID);

	// create the d3d texture
	createTexture(device);

	return true;
}

void Image::createTexture(ID3D12Device* device)
{

	DXGI_FORMAT dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// create image
	
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(dxFormat, width, height),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture2D)
		);

	// create intermediate image
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(
			GetRequiredIntermediateSize(texture2D.Get(), 0, 1)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadText)
		);
}

void Image::uploadTexture(ID3D12GraphicsCommandList* commandList)
{
	// update subdata
	D3D12_SUBRESOURCE_DATA subData;
	subData.pData = data;
	subData.RowPitch = width * 4;
	subData.SlicePitch = subData.RowPitch * height;

	// upload the texture
	UpdateSubresources<1>(commandList, texture2D.Get(), uploadText.Get(), 0, 0, 1, &subData);

	// set the barrier
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture2D.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
}

ILubyte* Image::getData()
{
	return data;
}
