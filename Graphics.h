#ifndef GRPAHICS_H
#define GRAPHICS_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include "ObjectFileLoader.h"
#include "Manager.h"

using namespace Microsoft::WRL;

#define CAM_DELTA .1f

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
		CS_BVH_CONSTRUCTION_P1,
		CS_BVH_CONSTRUCTION_P2,
		CS_BVH_CONSTRUCTION_TEST,
		CS_RAY_TRACE_TRAVERSAL,
		CS_COUNT
	};

	// setup the 
	enum BVHUAV : UINT32
	{
		UAV_VERTS = 0,
		UAV_BVHTREE,
		UAV_TRANSFER_BUFFER,
		UAV_NUM_ONES_BUFFER,
		UAV_RADIXI_BUFFER,
		UAV_OUTPUT_TEX,
		UAV_DEBUG_VAR,
		UAV_COUNT
	};

	// static globals
	static const UINT numFrames = 2;
	static const UINT numSRVHeaps = 3;
	static const UINT numUAVHeaps = UAV_COUNT;
	static const UINT numCBVHeaps = 2;

	// buffers
	ComPtr<ID3D12Resource> bufferCS[numUAVHeaps], zeroBuffer,
		debugBuffer, bufferCB[numCBVHeaps],
		plainVCB;

	D3D12_VERTEX_BUFFER_VIEW plainVB;

	XMFLOAT3 plainVerts[6] = {
		XMFLOAT3(1, 1, 0), XMFLOAT3(1, -1, 0), XMFLOAT3(-1, 1, 0),
		XMFLOAT3(-1, -1, 0), XMFLOAT3(-1, 1, 0), XMFLOAT3(1, -1, 0)
	};

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

	struct Box
	{
		XMFLOAT3 bbMin, bbMax;
	};

	struct NODE
	{
		int parent;
		int childL, childR;

		UINT code;

		// bounding box calc
		Box bbox;

		// index start value
		UINT index;
	};

	struct RESTART_BUFFER
	{
		UINT restart;
	};
	
	struct WORLD_POS
	{
		XMMATRIX worldViewProjection;
		XMMATRIX world;
	};

	struct RAY_TRACE_BUFFER
	{
		UINT numGrps, numObjects;
		UINT screenWidth, screenHeight;
		XMFLOAT3 sceneBBMin;
		UINT numIndices;
		XMFLOAT3 sceneBBMax;
	};

	// constant buffers
	WORLD_POS* worldPosCB;
	RAY_TRACE_BUFFER* rayTraceCB;

	RESTART_BUFFER* bufferRestartData;

	// view params
	XMMATRIX world, view, projection, worldViewProjection;

	const XMVECTOR origEye{ 0.0f, 10.0f, -100.0f };
	XMVECTOR eye = origEye;
	XMVECTOR at{ 0.0f, 0.0f, 0.0f };
	XMVECTOR up{ 0.0f, 1.f, 0.0f };

	float yAngle = 0, xAngle = 0;

	// locations in the root parameter table
	enum RootParameters : UINT32
	{
		rpCB0 = 0,
		rpCB1,
		rpSRV,
		rpUAV,
		rpCount
	};
};

#endif