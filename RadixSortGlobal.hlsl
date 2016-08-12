#ifndef RADIX_SORT_GLOBAL
#define RADIX_SORT_GLOBAL

#define NUM_THREADS 128
#define DATA_SIZE (NUM_THREADS << 1)

#define DEBUG

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
/*
cbuffer RESTART_BUFFER : register(b1)
{
	uint restart;
};*/

StructuredBuffer<VERTEX> verts : register(t0);
StructuredBuffer<uint> indices : register(t1);
StructuredBuffer<uint> restart : register(t2);

RWStructuredBuffer<uint> codes : register(u0);
RWStructuredBuffer<uint> sortedIndex : register(u1);
RWStructuredBuffer<uint> sortedIndexBackBuffer : register(u2);
RWStructuredBuffer<uint> transferBuffer : register(u3);
RWStructuredBuffer<uint> numOnesBuffer : register(u4);
RWStructuredBuffer<uint> phaseBuffer : register(u5);
RWStructuredBuffer<uint> radixiBuffer : register(u6);

//groupshared uint phase;
groupshared uint phase, radixi;

groupshared uint positionNotPresent[DATA_SIZE];
groupshared uint netOnes, numPrecOnes, totalOnes;

#endif