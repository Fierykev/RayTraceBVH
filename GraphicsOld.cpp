#include <d3dcompiler.h>
#include "d3dx12.h"
#include "Graphics.h"
#include "Helper.h"
#include "Window.h"
#include "Shader.h"

// TMP
#include <iostream>

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
	// record the render commands

	populateCommandList();

	// run the commands

	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// show the frame

	ThrowIfFailed(swapChain->Present(1, 0));

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
	cbvHeapDesc.NumDescriptors = numSRVHeaps + numUAVHeaps;
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
	const UINT numThreads = 128.f;
	const UINT numGrps = ceil(obj.getNumVertices() / (numThreads * 2.f));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMFLOAT3) * numGrps),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferCS[0])));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMFLOAT3) * numGrps),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferCS[1])));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int) * obj.getNumIndices() / 3),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferCS[2])));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int) * obj.getNumIndices() / 3),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferCS[3])));

	// load the shaders
	string dataVS, dataPS, dataPRCS, dataMCCS, dataRSCS;

	// file locations of each shader
#ifdef _DEBUG
#ifdef x64
	ThrowIfFailed(ReadCSO("../x64/Debug/RayTraceBVHVS.cso", dataVS));

	ThrowIfFailed(ReadCSO("../x64/Debug/RayTraceBVHPS.cso", dataPS));

	ThrowIfFailed(ReadCSO("../x64/Debug/ParallelReductionCS.cso", dataPRCS));

	ThrowIfFailed(ReadCSO("../x64/Debug/MortonCodesCS.cso", dataMCCS));

	ThrowIfFailed(ReadCSO("../x64/Debug/RadixSortCS.cso", dataRSCS));
#else
	// TODO: FILL IN LATER FOR x32
#endif
#else
#ifdef x64
	ThrowIfFailed(ReadCSO("../x64/Release/RayTraceBVHVS.cso", dataVS));

	ThrowIfFailed(ReadCSO("../x64/Release/RayTraceBVHPS.cso", dataPS));

	ThrowIfFailed(ReadCSO("../x64/Release/RayTraceBVHCS.cso", dataCS));
#else
	// TODO: FILL IN LATER FOR x32
#endif
#endif

	// setup constant buffer and descriptor tables

	CD3DX12_DESCRIPTOR_RANGE ranges[2];
	CD3DX12_ROOT_PARAMETER rootParameters[3];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numSRVHeaps, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numUAVHeaps, 0);
	rootParameters[rpCB].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[rpSRV].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[rpUAV].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);

	// empty root signature

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters) - 1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	// create compute signiture and change visibility

	rootParameters[rpSRV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

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

	// create the compute pipeline (parallel reduction)
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = computeRootSignature.Get();
	computePsoDesc.CS = { reinterpret_cast<UINT8*>((void*)dataPRCS.c_str()), dataPRCS.length() };

	ThrowIfFailed(device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&computeStatePR)));

	// create the compute pipeline (morton codes)

	computePsoDesc.pRootSignature = computeRootSignature.Get();
	computePsoDesc.CS = { reinterpret_cast<UINT8*>((void*)dataMCCS.c_str()), dataMCCS.length() };

	ThrowIfFailed(device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&computeStateMC)));

	// create the compute pipeline (radix sort)

	computePsoDesc.pRootSignature = computeRootSignature.Get();
	computePsoDesc.CS = { reinterpret_cast<UINT8*>((void*)dataRSCS.c_str()), dataRSCS.length() };

	ThrowIfFailed(device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&computeStateRS)));

	// create command list

	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex].Get(), nullptr, IID_PPV_ARGS(&commandList)));

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

	const UINT numThreads = 128.f;
	const UINT numGrps = ceil(obj.getNumVertices() / (numThreads * 2.f));

	// setup vertex data for transfer

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = obj.getNumVertices();
	srvDesc.Buffer.StructureByteStride = sizeof(Vertex);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(csuHeap->GetCPUDescriptorHandleForHeapStart());
	device->CreateShaderResourceView(obj.getVertexMappedBuffer(), &srvDesc, srvHandle0);

	// setup index data for transfer

	srvDesc.Buffer.NumElements = obj.getNumIndices();
	srvDesc.Buffer.StructureByteStride = sizeof(unsigned int);

	srvHandle0.Offset(1, csuDescriptorSize);
	device->CreateShaderResourceView(obj.getIndexMappedBuffer(), &srvDesc, srvHandle0);

	// setup buffer

	srvDesc.Buffer.NumElements = numGrps;
	srvDesc.Buffer.StructureByteStride = sizeof(XMFLOAT3);

	srvHandle0.Offset(1, csuDescriptorSize);
	device->CreateShaderResourceView(bufferCS[0].Get(), &srvDesc, srvHandle0);

	srvHandle0.Offset(1, csuDescriptorSize);
	device->CreateShaderResourceView(bufferCS[1].Get(), &srvDesc, srvHandle0);

	srvDesc.Buffer.NumElements = obj.getNumIndices() / 3;
	srvDesc.Buffer.StructureByteStride = sizeof(unsigned int);

	srvHandle0.Offset(1, csuDescriptorSize);
	device->CreateShaderResourceView(bufferCS[2].Get(), &srvDesc, srvHandle0);

	srvHandle0.Offset(1, csuDescriptorSize);
	device->CreateShaderResourceView(bufferCS[3].Get(), &srvDesc, srvHandle0);

	// create compute resources

	D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_COMPUTE, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };
	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&computeCommandQueue)));
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&computeAllocator)));
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, computeAllocator.Get(), nullptr, IID_PPV_ARGS(&computeCommandList)));
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&threadFences)));

	// set the shader
	ID3D12GraphicsCommandList* pCommandList = computeCommandList.Get();

	pCommandList->SetPipelineState(computeStatePR.Get());
	pCommandList->SetComputeRootSignature(computeRootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { csuHeap.Get() };
	pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), 0, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), 2, csuDescriptorSize);

	pCommandList->SetComputeRootDescriptorTable(rpSRV, srvHandle); // input
	pCommandList->SetComputeRootDescriptorTable(rpUAV, uavHandle); // output

	pCommandList->Dispatch(numGrps, 1, 1);

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

	// setup shader for computing morton codes
	// set the shader
	pCommandList->Reset(computeAllocator.Get(), computeStatePR.Get());

	pCommandList->SetPipelineState(computeStateMC.Get());
	pCommandList->SetComputeRootSignature(computeRootSignature.Get());

	pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	uavHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(csuHeap->GetGPUDescriptorHandleForHeapStart(), 4, csuDescriptorSize);

	pCommandList->SetComputeRootDescriptorTable(rpUAV, uavHandle); // input
	pCommandList->SetComputeRootDescriptorTable(rpSRV, srvHandle); // output

	pCommandList->Dispatch(obj.getNumIndices() / 3, 1, 1);

	// Close and execute the command list.
	ThrowIfFailed(pCommandList->Close());

	computeCommandQueue->ExecuteCommandLists(1, ppCommandLists);

	m_threadFenceValues = 0;
	m_threadFenceEvents = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_threadFenceEvents == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	// wait for the compute shader to process the commands

	threadFenceValue = InterlockedIncrement(&m_threadFenceValues);
	ThrowIfFailed(computeCommandQueue->Signal(threadFences.Get(), threadFenceValue));
	ThrowIfFailed(threadFences->SetEventOnCompletion(threadFenceValue, m_threadFenceEvents));
	WaitForSingleObject(m_threadFenceEvents, INFINITE);

	// setup shader for radix sort

	// set the shader
	pCommandList->Reset(computeAllocator.Get(), computeStateMC.Get());

	pCommandList->SetPipelineState(computeStateRS.Get());
	pCommandList->SetComputeRootSignature(computeRootSignature.Get());

	pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	uavHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(csuHeap->GetGPUDescriptorHandleForHeapStart(), 3, csuDescriptorSize);

	pCommandList->SetComputeRootDescriptorTable(rpUAV, uavHandle);

	pCommandList->Dispatch(numGrps, 1, 1);

	// Close and execute the command list.
	ThrowIfFailed(pCommandList->Close());

	computeCommandQueue->ExecuteCommandLists(1, ppCommandLists);

	m_threadFenceValues = 0;
	m_threadFenceEvents = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_threadFenceEvents == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	// wait for the compute shader to process the commands

	threadFenceValue = InterlockedIncrement(&m_threadFenceValues);
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

	CD3DX12_GPU_DESCRIPTOR_HANDLE csuHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), 2, csuDescriptorSize);

	commandList->SetGraphicsRootDescriptorTable(rpSRV, csuHandle);
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
	commandList->DrawIndexedInstanced(obj.getNumIndices(), 1, 0, 0, 0);

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