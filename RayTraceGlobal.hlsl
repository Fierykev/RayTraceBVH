#ifndef RADIX_SORT_GLOBAL
#define RADIX_SORT_GLOBAL

#define NUM_THREADS 128
#define DATA_SIZE (NUM_THREADS << 1)

#define MAX_TEXTURES 10

#define INTENSITY_MIN 0
#define REFLECTION_DECAY 1
#define REFRACTION_DECAY 1

// shapes

struct Box
{
	float3 bbMin, bbMax;
};

struct Ray
{
	float3 origin;
	float3 direction;

	float3 invDirection;
};

struct RayPresent
{
	float intensity;
	Ray ray;
	float4 color;
};

// buffer structs

struct Node
{
	int parent;
	int childL, childR;

	uint code;

	// bounding box calc
	Box bbox;

	// index start value
	uint index;
};

struct Vertex
{
	float3 position;
	float3 normal;
	float2 texcoord;
};

struct Material
{
	float4 ambient;
	float4 diffuse;
	float4 specular;

	float shininess;
	float opticalDensity;
	float alpha;
	bool specularb;

	int texNum;
};

struct Triangle
{
	Vertex verts[3];
};

struct ColTri
{
	bool collision;
	float distance;
	uint index;
	Triangle tri;
};

cbuffer WORLD_POS : register(b0)
{
	matrix worldViewProjection;
	matrix worldView;
};

cbuffer RAY_TRACE_BUFFER : register(b1)
{
	// pack 1
	uint numGrps, numObjects;
	uint screenWidth, screenHeight;

	// pack 2
	float3 sceneBBMin;
	uint numIndices;
	
	// pack 3
	float3 sceneBBMax;
};

StructuredBuffer<Vertex> verts : register(t0);
StructuredBuffer<uint> indices : register(t1);
StructuredBuffer<uint> matIndices : register(t2);
StructuredBuffer<Material> mat : register(t3);
Texture2D diffuseTex[MAX_TEXTURES] : register(t4);

RWStructuredBuffer<Node> BVHTree : register(u0);
RWStructuredBuffer<uint> transferBuffer : register(u1);
RWStructuredBuffer<uint> numOnesBuffer : register(u2);
RWStructuredBuffer<uint> radixiBuffer : register(u3);
RWStructuredBuffer<RayPresent> reflectRay : register(u4);
RWStructuredBuffer<RayPresent> refractRay : register(u5);

SamplerState compSample : register(s0);

groupshared uint radixi;

groupshared uint positionNotPresent[DATA_SIZE];
groupshared uint netOnes, numPrecOnes;

uint rand(uint lfsr)
{
	return ((uint)(lfsr / 65536) % 32768);
}

float3 minUnion(float3 data1, float3 data2)
{
	return float3(min(data1.x, data2.x),
		min(data1.y, data2.y), min(data1.z, data2.z));
}

float3 maxUnion(float3 data1, float3 data2)
{
	return float3(max(data1.x, data2.x),
		max(data1.y, data2.y), max(data1.z, data2.z));
}

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