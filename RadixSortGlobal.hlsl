#ifndef RADIX_SORT_GLOBAL
#define RADIX_SORT_GLOBAL

#define NUM_THREADS 128
#define DATA_SIZE (NUM_THREADS << 1)

#define DEBUG

struct NODE
{
	int parent;
	int childL, childR;

	uint code;
};

struct VERTEX
{
	float3 position;
	float3 normal;
	float2 texcoord;
};

cbuffer CONSTANT_BUFFER : register(b0)
{
	uint numGrps;
	float3 max, min;
};

StructuredBuffer<VERTEX> verts : register(t0);
StructuredBuffer<uint> indices : register(t1);

RWStructuredBuffer<uint> codes : register(u0);
RWStructuredBuffer<uint> sortedIndex : register(u1);
RWStructuredBuffer<uint> sortedIndexBackBuffer : register(u2);
RWStructuredBuffer<uint> transferBuffer : register(u3);
RWStructuredBuffer<uint> numOnesBuffer : register(u4);
RWStructuredBuffer<uint> radixiBuffer : register(u5);

//groupshared uint phase;
groupshared uint radixi;

groupshared uint positionNotPresent[DATA_SIZE];
groupshared uint netOnes, numPrecOnes, totalOnes;

void getRadixi(uint groupThreadID, uint3 groupID)
{
	// get the data
	if (groupThreadID == 0)
		radixi = radixiBuffer[groupID.x];
	
	GroupMemoryBarrierWithGroupSync();
}

void incRadixi(uint groupThreadID, uint3 groupID)
{
	// increment the data
	if (groupThreadID == 0)
		radixiBuffer[groupID.x] = radixi + 1;
}

#endif