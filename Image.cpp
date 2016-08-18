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

Image::~Image()
{
	// unbind and delete

	ilBindImage(0);
	ilDeleteImage(imageID);
}

bool Image::loadImage(ID3D12Device* device, const wchar_t* filename)
{
	ILuint ImgId = 0;

	if (!ilLoadImage(filename))
		return false;

	width = ilGetInteger(IL_IMAGE_WIDTH);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	format = ilGetInteger(IL_IMAGE_FORMAT);

	// get the image data and unbind
	data = ilGetData();

	// create the d3d texture
	createTexture(device);

	return true;
}

void Image::createTexture(ID3D12Device* device)
{
	subData.pData = data;

	DXGI_FORMAT dxFormat;
	
	switch (format)
	{
	case IL_COLOR_INDEX:
		// TODO: FIX
		dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		subData.RowPitch = width * 3;
		subData.SlicePitch = width * height * 3;

		break;
	case IL_RGB:

		dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		subData.RowPitch = width * 3;
		subData.SlicePitch = width * height * 3;

		break;
	case IL_RGBA:

		dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		subData.RowPitch = width * 4;
		subData.SlicePitch = width * height * 4;

		break;
	default:

		cout << "Add format " << format << endl;
		system("PAUSE");
		exit(-1);
	}

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
	// upload the texture
	//UpdateSubresources(commandList, texture2D.Get(), uploadText.Get(), 0, 0, 1, &subData);

	// set the barrier
	//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture2D.Get(),
		//D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
}

ILubyte* Image::getData()
{
	return data;
}
