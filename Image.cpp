#include "d3dx12.h"
#include "Image.h"
#include "Helper.h"

#include "Graphics.h"

#define TEX_WIDTH 64
#define TEX_HEIGHT 64
#define TEX_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM_SRGB

// declare static vars
UINT Image::descriptorSize;
CD3DX12_CPU_DESCRIPTOR_HANDLE Image::srvTexStart;
UINT Image::numResources;

Image::Image()
{
	
}

void Image::deleteImage()
{
	// delete the data
	if (data)
	{
		delete[] data;

		// TODO: figure out how to replace buffer
		//uploadBufferSize -= intermediateSize;

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

	// create the texture
	createTexture(device);

	return true;
}

void Image::createTexture(ID3D12Device* device)
{
	// create image
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = TEX_FORMAT;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		nullptr,
		IID_PPV_ARGS(&texture2D)
		));

	// add to upload buffer
	subresourceNum =
		textureDesc.DepthOrArraySize * textureDesc.MipLevels;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(
			GetRequiredIntermediateSize(texture2D.Get(),
				0, subresourceNum)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texture2DUpload)
		));

	// create the srv for this texture
	D3D12_SHADER_RESOURCE_VIEW_DESC matDesc = {};
	matDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	matDesc.Format = TEX_FORMAT;
	matDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	matDesc.Texture2D.MipLevels = 1;

	// set resource location
	prevResourceNum = numResources;

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvPos(srvTexStart, prevResourceNum, descriptorSize);

	device->CreateShaderResourceView(texture2D.Get(), &matDesc, srvPos);

	// update num of resources
	numResources++;
}

void Image::uploadTexture(ID3D12GraphicsCommandList* commandList)
{
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = data;
	textureData.RowPitch = width * 4;
	textureData.SlicePitch = textureData.RowPitch * height;

	UpdateSubresources(commandList, texture2D.Get(),
		texture2DUpload.Get(), 0, 0, subresourceNum, &textureData);

	// transition phase of texture
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture2D.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
}

ILubyte* Image::getData()
{
	return data;
}

void Image::initDevil()
{
	ilInit();
}

void Image::setSRVBase(CD3DX12_CPU_DESCRIPTOR_HANDLE srvTexStartPass, UINT descriptorSizePass)
{
	// copy over the start pos for the texture base
	srvTexStart = srvTexStartPass;
	descriptorSize = descriptorSizePass;

	// reset number of resources
	numResources = 0;
}

void Image::createSampler(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE iDesc)
{
	// Describe and create a sampler.
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	device->CreateSampler(&samplerDesc, iDesc);
}