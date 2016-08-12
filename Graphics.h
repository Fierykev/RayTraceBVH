#ifndef GRPAHICS_H
#define GRAPHICS_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include "ObjectFileLoader.h"
#include "Manager.h"

using namespace Microsoft::WRL;

class Graphics : public Manager
{
public:
	// use Manager's constructor
	Graphics(std::wstring title, unsigned int width, unsigned int height);

	virtual void onInit();
	virtual void onUpdate();
	virtual void onRender();
	virtual void onDestroy();
	virtual void onKeyDown(UINT8 key);
	virtual void onKeyUp(UINT8 key);

private:
	// methods
	void loadPipeline();
	void loadAssets();
	void computeBVH();
	void populateCommandList();
	void moveToNextFrame();
	void waitForGpu();

	enum ComputeShader : UINT32
	{
		CS_MORTON_CODES = 0,
		CS_RADIX_SORT_P1,
		CS_RADIX_SORT_P2,
		CS_RADIX_SORT_TEST,
		CS_COUNT
	};

	// setup the 
	enum BVHUAV : UINT32
	{
		UAV_BVHTREE = 0,
		UAV_TRANSFER_BUFFER,
		UAV_NUM_ONES_BUFFER,
		UAV_RADIXI_BUFFER,
		UAV_COUNT
	};

	// static globals
	static const UINT numFrames = 2;
	static const UINT numSRVHeaps = 2;
	static const UINT numUAVHeaps = UAV_COUNT;
	static const UINT numCBVHeaps = 1;

	// buffers
	ComPtr<ID3D12Resource> bufferCS[numUAVHeaps], zeroBuffer, bufferCB[1];

	// fences
	UINT frameIndex;
	UINT64 fenceVal[numFrames];

	// pipeline objects
	D3D12_VIEWPORT viewport;
	ComPtr<ID3D12Device> device;
	D3D12_RECT scissorRect;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12RootSignature> computeRootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12PipelineState> computeStatePR;
	ComPtr<ID3D12PipelineState> computeStateMC;
	ComPtr<ID3D12PipelineState> computeStateCS[CS_COUNT];
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<IDXGISwapChain3> swapChain;

	// per frame vars
	ComPtr<ID3D12Resource> renderTarget[numFrames];
	ComPtr<ID3D12CommandAllocator> commandAllocator[numFrames];

	// heaps
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12DescriptorHeap> csuHeap;
	UINT rtvDescriptorSize;
	UINT csuDescriptorSize;

	// synchronized objects
	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent;
	HANDLE swapChainEvent;

	// model
	ObjLoader obj;

	struct NODE
	{
		int parent;
		int childL, childR;

		UINT code;
	};

	struct RESTART_BUFFER
	{
		UINT restart;
	};

	struct CONSTANT_BUFFER
	{
		UINT numGrps, numObjects;
		XMFLOAT3 max, min;
	};

	// buffer map
	CONSTANT_BUFFER* bufferData;

	RESTART_BUFFER* bufferRestartData;

	// locations in the root parameter table
	enum RootParameters : UINT32
	{
		rpCB0 = 0,
		//rpCB1,
		rpSRV,
		rpUAV,
		rpCount
	};
};

#endif