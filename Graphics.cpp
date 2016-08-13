#include <d3dcompiler.h>
#include "d3dx12.h"
#include "Graphics.h"
#include "Helper.h"
#include "Window.h"
#include "Shader.h"

// TMP
#include <iostream>
#include <ctime>

#define DATA_SIZE 256

// TMP
UINT numGrps;
std::clock_t start;
double fps = 0, frames = 0;

Graphics::Graphics(std::wstring title, unsigned int width, unsigned int height)
	: Manager(title, width, height),
	frameIndex(0)
{
	ZeroMemory(fenceVal, sizeof(fenceVal));
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MaxDepth = 1.0f;

	scissorRect.right = static_cast<LONG>(width);
	scissorRect.bottom = static_cast<LONG>(height);
}

void Graphics::onInit()
{
	loadPipeline();
	loadAssets();
}

void Graphics::onUpdate()
{
	// wait for the last present
	
	WaitForSingleObjectEx(swapChainEvent, 100, FALSE);

	// run the compute bbh

	computeBVH();
}

void Graphics::onRender()
{
	double delta = (std::clock() - start) / (double)CLOCKS_PER_SEC;

	if (delta > 1.0)
	{
		fps = frames / delta;

		frames = 0;

		start = std::clock();
	}

	// record the render commands

	populateCommandList();

	// run the commands

	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// show the frame

	ThrowIfFailed(swapChain->Present(1, 0));

	if (frames == 0)
		std::cout << "FPS: " << fps << '\n';

	frames++;

	// go to the next frame

	moveToNextFrame();
}

void Graphics::onDestroy()
{
	// wait for the GPU to finish

	waitForGpu();

	// close the elements

	CloseHandle(fenceEvent);
}

void Graphics::loadPipeline()
{
#if defined(_DEBUG)

	// Enable debug in direct 3D
	ComPtr<ID3D12Debug> debugController;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		debugController->EnableDebugLayer();

#endif

	// create the graphics infrastructure
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

	// check for wraper device

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
			));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
			));
	}

	// create the command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	// create the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = numFrames;
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = Window::getWindow();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	ComPtr<IDXGISwapChain> swapChaintmp;
	ThrowIfFailed(factory->CreateSwapChain(
		commandQueue.Get(),
		&swapChainDesc,
		&swapChaintmp
		));

	ThrowIfFailed(swapChaintmp.As(&swapChain));

	// does not support full screen transitions for now TODO: fix this

	ThrowIfFailed(factory->MakeWindowAssociation(Window::getWindow(), DXGI_MWA_NO_ALT_ENTER));
	
	// store the frame and swap chain

	frameIndex = swapChain->GetCurrentBackBufferIndex();
	swapChainEvent = swapChain->GetFrameLatencyWaitableObject();

	// create descriptor heaps (RTV and CBV / SRV / UAV)

	// RTV

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = numFrames;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// CBV / SRV / UAV

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors = numSRVHeaps + numUAVHeaps + numCBVHeaps;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&csuHeap)));
	
	csuDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// frame resource

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

	// setup RTV and command allocator for the frames

	for (UINT n = 0; n < numFrames; n++)
	{
		ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTarget[n])));
		device->CreateRenderTargetView(renderTarget[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);

		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[n])));
	}
}

void Graphics::loadAssets()
{
	// load the mesh file to be displayed
	obj.Load("Obj/Test.obj", device.Get());

	// setup buffers for shaders
	const UINT numThreads = 128;
	/*const UINT */numGrps = (UINT) ceil(obj.getNumIndices() / (double)DATA_SIZE);
	
	// load the shaders
	string dataVS, dataPS, dataCS[CS_COUNT];

	// file locations of each shader
#ifdef _DEBUG
#ifdef x64
	ThrowIfFailed(ReadCSO("../x64/Debug/RayTraceBVHVS.cso", dataVS));

	ThrowIfFailed(ReadCSO("../x64/Debug/RayTraceBVHPS.cso", dataPS));

	ThrowIfFailed(ReadCSO("../x64/Debug/GenMortonCodes.cso", dataCS[CS_MORTON_CODES]));

	ThrowIfFailed(ReadCSO("../x64/Debug/RadixSortP1.cso", dataCS[CS_RADIX_SORT_P1]));

	ThrowIfFailed(ReadCSO("../x64/Debug/RadixSortP2.cso", dataCS[CS_RADIX_SORT_P2]));

	ThrowIfFailed(ReadCSO("../x64/Debug/RadixSortTest.cso", dataCS[CS_RADIX_SORT_TEST]));

	ThrowIfFailed(ReadCSO("../x64/Debug/BVHConstruct.cso", dataCS[CS_BVH_CONSTRUCTION]));

	ThrowIfFailed(ReadCSO("../x64/Debug/BVHConstructTest.cso", dataCS[CS_BVH_CONSTRUCTION_TEST]));
#else
	// TODO: FILL IN LATER FOR x32
#endif
#else
#ifdef x64
	ThrowIfFailed(ReadCSO("../x64/Release/RayTraceBVHVS.cso", dataVS));

	ThrowIfFailed(ReadCSO("../x64/Release/RayTraceBVHPS.cso", dataPS));

	ThrowIfFailed(ReadCSO("../x64/Release/GenMortonCodes.cso", dataCS[CS_MORTON_CODES]));

	ThrowIfFailed(ReadCSO("../x64/Release/RadixSortP1.cso", dataCS[CS_RADIX_SORT_P1]));

	ThrowIfFailed(ReadCSO("../x64/Release/RadixSortP2.cso", dataCS[CS_RADIX_SORT_P2]));

	ThrowIfFailed(ReadCSO("../x64/Release/RadixSortTest.cso", dataCS[CS_RADIX_SORT_TEST]));

	ThrowIfFailed(ReadCSO("../x64/Release/BVHConstruct.cso", dataCS[CS_BVH_CONSTRUCTION]));
#else
	// TODO: FILL IN LATER FOR x32
#endif
#endif
	
	// setup constant buffer and descriptor tables
	
	CD3DX12_DESCRIPTOR_RANGE ranges[2];
	CD3DX12_ROOT_PARAMETER rootParameters[3];

	//ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, numCBVHeaps, 0);
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numSRVHeaps, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numUAVHeaps, 0);
	rootParameters[rpCB0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	//rootParameters[rpCB1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
	//InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	//rootParameters[rpCB].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[rpSRV].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[rpUAV].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);

	// empty root signature

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	// was cof -1

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	// create compute signiture
	CD3DX12_ROOT_SIGNATURE_DESC computeRootSignatureDesc(_countof(rootParameters), rootParameters, 0, nullptr);
	ThrowIfFailed(D3D12SerializeRootSignature(&computeRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&computeRootSignature)));

	// vertex input layout

	const D3D12_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { layout, _countof(layout) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataVS.c_str()), dataVS.length() };
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)dataPS.c_str()), dataPS.length() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));

	// create the compute pipeline (radix sort)
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};

	computePsoDesc.pRootSignature = computeRootSignature.Get();

	for (unsigned int i = 0; i < CS_COUNT; i++)
	{
		computePsoDesc.CS = { reinterpret_cast<UINT8*>((void*)dataCS[i].c_str()), dataCS[i].length() };

		ThrowIfFailed(device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&computeStateCS[i])));
	}

	// create command list

	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex].Get(), nullptr, IID_PPV_ARGS(&commandList)));
		
	// UAV
	
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(NODE) * numGrps * DATA_SIZE * 2, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&bufferCS[UAV_BVHTREE])));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int) * numGrps * DATA_SIZE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&bufferCS[UAV_TRANSFER_BUFFER])));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int) * numGrps * DATA_SIZE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&bufferCS[UAV_NUM_ONES_BUFFER])));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int) * numGrps, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&bufferCS[UAV_RADIXI_BUFFER])));

	// zero bool buffer
	
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * numGrps),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&zeroBuffer)));
		
	// constant buffer

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(CONSTANT_BUFFER) + 255) & ~255),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferCB[0])));

	// setup data to transfer

	// setup vertex data for transfer

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = obj.getNumVertices();
	uavDesc.Buffer.StructureByteStride = obj.getNumVertices();
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = obj.getNumVertices();
	srvDesc.Buffer.StructureByteStride = sizeof(Vertex);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	// TODO: check if below is needed

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(csuHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(csuHeap->GetCPUDescriptorHandleForHeapStart(), numCBVHeaps, csuDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle0(csuHeap->GetCPUDescriptorHandleForHeapStart(), numCBVHeaps + numSRVHeaps, csuDescriptorSize);

	// set up zero buffer

	CD3DX12_RANGE readRange(0, 0);
	
	UINT* boolBufferData;

	zeroBuffer->Map(0, &readRange, reinterpret_cast<void**>(&boolBufferData));
	ZeroMemory(boolBufferData, sizeof(UINT) * numGrps);
	zeroBuffer->Unmap(0, nullptr);
	
	// set up the constant buffer

	D3D12_CONSTANT_BUFFER_VIEW_DESC cdesc;
	cdesc.BufferLocation = bufferCB[0]->GetGPUVirtualAddress();
	cdesc.SizeInBytes = (sizeof(CONSTANT_BUFFER) + 255) & ~255;

	device->CreateConstantBufferView(&cdesc, cbvHandle0);

	// map resources for upload
	bufferCB[0]->Map(0, &readRange, (void**)&bufferData);
	bufferData->numGrps = numGrps;
	bufferData->numObjects = numGrps * DATA_SIZE;
	bufferData->sceneBBMax = XMFLOAT3(36, 42, 44);
	bufferData->sceneBBMin = XMFLOAT3(-51, -2, -43);
	bufferCB[0]->Unmap(0, nullptr);

	// setup vertex and index data (srv)
	device->CreateShaderResourceView(obj.getVertexMappedBuffer(), &srvDesc, srvHandle0);
	
	// setup index data for transfer

	srvDesc.Buffer.NumElements = (UINT)obj.getNumIndices();
	srvDesc.Buffer.StructureByteStride = sizeof(unsigned int);
	
	srvHandle0.Offset(1, csuDescriptorSize);
	device->CreateShaderResourceView(obj.getIndexMappedBuffer(), &srvDesc, srvHandle0);

	// setup buffer (uav)

	// BVH tree
	
	uavDesc.Buffer.NumElements = numGrps * DATA_SIZE * 2;
	uavDesc.Buffer.StructureByteStride = sizeof(NODE);

	device->CreateUnorderedAccessView(bufferCS[UAV_BVHTREE].Get(), nullptr, &uavDesc, uavHandle0);

	// transfer buffer

	uavDesc.Buffer.NumElements = numGrps * DATA_SIZE;
	uavDesc.Buffer.StructureByteStride = sizeof(unsigned int);

	uavHandle0.Offset(1, csuDescriptorSize);
	device->CreateUnorderedAccessView(bufferCS[UAV_TRANSFER_BUFFER].Get(), nullptr, &uavDesc, uavHandle0);
	
	// one's buffer (used for sorting)

	uavDesc.Buffer.NumElements = numGrps;
	uavDesc.Buffer.StructureByteStride = sizeof(unsigned int);

	uavHandle0.Offset(1, csuDescriptorSize);
	device->CreateUnorderedAccessView(bufferCS[UAV_NUM_ONES_BUFFER].Get(), nullptr, &uavDesc, uavHandle0);
	
	// radixi

	uavDesc.Buffer.NumElements = numGrps;
	uavDesc.Buffer.StructureByteStride = sizeof(unsigned int);

	uavHandle0.Offset(1, csuDescriptorSize);
	device->CreateUnorderedAccessView(bufferCS[UAV_RADIXI_BUFFER].Get(), nullptr, &uavDesc, uavHandle0);

	// close the command list until things are added

	ThrowIfFailed(commandList->Close());

	// synchronize objects and wait until assets are uploaded to the GPU

	ThrowIfFailed(device->CreateFence(fenceVal[frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	fenceVal[frameIndex]++;

	// create an event handler

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (fenceEvent == nullptr)
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));

	// wait for the command list to finish being run on the GPU

	waitForGpu();
}

void Graphics::computeBVH()
{
	ComPtr<ID3D12CommandQueue> computeCommandQueue;
	ComPtr<ID3D12CommandAllocator> computeAllocator;
	ComPtr<ID3D12GraphicsCommandList> computeCommandList;
	ComPtr<ID3D12Fence> threadFences;
	
	ComPtr<ID3D12Resource> outputBufferMax, outputBufferMin;

	const UINT numThreads = 128;
	//const UINT numGrps = ceil(obj.getNumIndices() / (numThreads * 2.f)) - 1; // -1 tmp

	// create compute resources

	D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_COMPUTE, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };
	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&computeCommandQueue)));
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&computeAllocator)));
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, computeAllocator.Get(), nullptr, IID_PPV_ARGS(&computeCommandList)));
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&threadFences)));

	// set the shader
	ID3D12GraphicsCommandList* pCommandList = computeCommandList.Get();
	
	pCommandList->SetPipelineState(computeStateCS[CS_MORTON_CODES].Get());
	pCommandList->SetComputeRootSignature(computeRootSignature.Get());

	// set constant buffer
	pCommandList->SetComputeRootConstantBufferView(rpCB0, bufferCB[0]->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* ppHeaps[] = { csuHeap.Get() };
	pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), 0, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), numCBVHeaps, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), numCBVHeaps + numSRVHeaps, csuDescriptorSize);

	//pCommandList->SetComputeRootDescriptorTable(rpCB, cbvHandle); // constant
	pCommandList->SetComputeRootDescriptorTable(rpSRV, srvHandle); // input
	pCommandList->SetComputeRootDescriptorTable(rpUAV, uavHandle); // output

	// setup the command list

	// morton code gen

	// reset to start over
	pCommandList->CopyBufferRegion(bufferCS[UAV_RADIXI_BUFFER].Get(), 0, zeroBuffer.Get(), 0, sizeof(UINT) * numGrps);

	pCommandList->Dispatch(numGrps, 1, 1); // TODO: ADD BACK NUMGRPS

	// barrier for morton codes and zero buffer
	const CD3DX12_RESOURCE_BARRIER mortonBarrier[] = {
		CD3DX12_RESOURCE_BARRIER::UAV(bufferCS[UAV_BVHTREE].Get()), // bvh tree
		CD3DX12_RESOURCE_BARRIER::UAV(bufferCS[UAV_RADIXI_BUFFER].Get()) // radixi
	};

	pCommandList->ResourceBarrier(_countof(mortonBarrier), mortonBarrier);
	
	// barriers for radix sort
	const CD3DX12_RESOURCE_BARRIER p1Barrier[] = {
		CD3DX12_RESOURCE_BARRIER::UAV(bufferCS[UAV_NUM_ONES_BUFFER].Get()), // numOnesBuffer
		CD3DX12_RESOURCE_BARRIER::UAV(bufferCS[UAV_TRANSFER_BUFFER].Get()) // transferBuffer
	};

	const CD3DX12_RESOURCE_BARRIER p2Barrier[] = {
		CD3DX12_RESOURCE_BARRIER::UAV(bufferCS[UAV_RADIXI_BUFFER].Get()), // radixi
		CD3DX12_RESOURCE_BARRIER::UAV(bufferCS[UAV_BVHTREE].Get()) // bvh tree
	};

	// run the sort
	for (unsigned int i = 0; i < 32; i++)
	{
		// set state to p1
		pCommandList->SetPipelineState(computeStateCS[CS_RADIX_SORT_P1].Get());

		// launch threads
		pCommandList->Dispatch(numGrps, 1, 1);
		
		// wait for UAV's to write
		pCommandList->ResourceBarrier(_countof(p1Barrier), p1Barrier);

		// set state to p2
		pCommandList->SetPipelineState(computeStateCS[CS_RADIX_SORT_P2].Get());

		// launch threads
		pCommandList->Dispatch(numGrps, 1, 1);

		// wait for UAV's to write
		pCommandList->ResourceBarrier(_countof(p2Barrier), p2Barrier);
	}

	/*
	// sync EVERYTHING

	pCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::UAV(NULL));

	// run debug (TMP)

	pCommandList->SetPipelineState(computeStateCS[CS_RADIX_SORT_TEST].Get());	
	pCommandList->Dispatch(1, 1, 1);
	*/

	// construct the bvh
	pCommandList->SetPipelineState(computeStateCS[CS_BVH_CONSTRUCTION].Get());

	// launch threads
	pCommandList->Dispatch(numGrps, 1, 1);

	// wait for UAV's to write
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(bufferCS[UAV_BVHTREE].Get()));

	// sync EVERYTHING

	pCommandList->ResourceBarrier(1,
	&CD3DX12_RESOURCE_BARRIER::UAV(NULL));

	// run debug (TMP)

	pCommandList->SetPipelineState(computeStateCS[CS_BVH_CONSTRUCTION_TEST].Get());
	pCommandList->Dispatch(1, 1, 1);

	// Close and execute the command list.
	ThrowIfFailed(pCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { pCommandList };

	computeCommandQueue->ExecuteCommandLists(1, ppCommandLists);

	UINT64 m_threadFenceValues = 0;
	HANDLE m_threadFenceEvents = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_threadFenceEvents == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	// wait for the compute shader to process the commands
	
	UINT64 threadFenceValue = InterlockedIncrement(&m_threadFenceValues);
	ThrowIfFailed(computeCommandQueue->Signal(threadFences.Get(), threadFenceValue));
	ThrowIfFailed(threadFences->SetEventOnCompletion(threadFenceValue, m_threadFenceEvents));
	WaitForSingleObject(m_threadFenceEvents, INFINITE);
}

void Graphics::populateCommandList()
{
	// reset the command allocator

	ThrowIfFailed(commandAllocator[frameIndex]->Reset());

	// reset the command list

	ThrowIfFailed(commandList->Reset(commandAllocator[frameIndex].Get(), pipelineState.Get()));

	// set the state
	
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { csuHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// setup the resources

	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), 0, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), numCBVHeaps, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), numCBVHeaps + numSRVHeaps, csuDescriptorSize);

	//CD3DX12_GPU_DESCRIPTOR_HANDLE csuHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), numCBVHeaps + numSRVHeaps, csuDescriptorSize);

	commandList->SetGraphicsRootDescriptorTable(rpSRV, srvHandle); // input
	commandList->SetGraphicsRootDescriptorTable(rpUAV, uavHandle); // output

	//commandList->SetGraphicsRootDescriptorTable(rpSRV, csuHandle);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	// set the back buffer as the render target

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// set the render target view

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// set the background

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// draw the object

	commandList->IASetPrimitiveTopology(obj.getPrimativeType());
	commandList->IASetVertexBuffers(0, 1, obj.getVertexBuffer());
	commandList->IASetIndexBuffer(obj.getIndexBuffer());
	commandList->DrawIndexedInstanced((UINT)obj.getNumIndices(), 1, 0, 0, 0);
	
	// do not present the back buffer

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// close the commands

	ThrowIfFailed(commandList->Close());

}

void Graphics::waitForGpu()
{
	// add a signal to the command queue
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceVal[frameIndex]));

	// wait until the signal is processed

	ThrowIfFailed(fence->SetEventOnCompletion(fenceVal[frameIndex], fenceEvent));
	WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

	// update the fence value

	fenceVal[frameIndex]++;
}

void Graphics::moveToNextFrame()
{
	// write signal command to the queue

	const UINT64 currentFenceValue = fenceVal[frameIndex];
	ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));

	// update the index

	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// wait if the frame has not been rendered yet

	if (fence->GetCompletedValue() < fenceVal[frameIndex])
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceVal[frameIndex], fenceEvent));
		WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
	}

	// set the fence value for the new frame

	fenceVal[frameIndex] = currentFenceValue + 1;
}

// keyboard

void Graphics::onKeyDown(UINT8 key)
{

}

void Graphics::onKeyUp(UINT8 key)
{

}