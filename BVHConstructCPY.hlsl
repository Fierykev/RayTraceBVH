#define NUM_THREADS 128
#define DATA_SIZE (NUM_THREADS << 1)

struct NODE
{
	uint index;
	uint parent;
	uint childL, childR;
};

struct VERTEX
{
	float3 position;
	float3 normal;
	float2 texcoord;
};

cbuffer CONSTANT_BUFFER : register(b0)
{
	uint numObjects;
};

StructuredBuffer<VERTEX> verts : register(t0);
StructuredBuffer<uint> indices : register(t1);
StructuredBuffer<uint> restart : register(t2);

RWStructuredBuffer<uint> codes : register(u0);
RWStructuredBuffer<uint> sortedIndex : register(u1);
RWStructuredBuffer<NODE> nodes : register(u2);
// size of number of objects = leaf node
// size of number of objects - 1 = internal nodes

/*
Gets number of leeading zeros by representing them
as ones pushed to the right.  Does not give meaningful
number but the relative output is correct.
*/

uint leadingZeroRel(uint data)
{
	// remove non-leading zeros

	data |= data >> 1;
	data |= data >> 2;
	data |= data >> 4;
	data |= data >> 8;
	data |= data >> 16;

	return ~data;
}

/*
Calculate the search range.
*/

uint2 calcRange(uint index)
{
	uint codeCurrent = codes[index],
		codeBefore = codes[index + 1],
		codeAfter = codes[index - 1];

	// get range direction
	int direction = sign(leadingZeroRel(codeCurrent ^ codeAfter)
		- leadingZeroRel(codeCurrent ^ codeBefore));

	// get upper bound of lenght range
	// TODO: look into using before and after 
	uint minCode = leadingZeroRel(codeCurrent ^ codes[index - direction]);

	uint upperBound = 2;

	// TODO: fix multiplication
	for (;
	minCode < leadingZeroRel(codeCurrent ^ codes[index + upperBound * direction]);
		upperBound <<= 2) {
	}

	// find lower bound
	uint delta = upperBound;

	uint tmp = 0;

	do
	{
		delta = (delta + 1) >> 1;

		if (leadingZeroRel(codeCurrent ^ codes[index + (tmp + delta) * direction]))
			tmp += delta;
	} while (1 < delta);

	uint lowerBound = index + tmp * direction;

	// find slice range
	uint leadingZero = leadingZeroRel(codeCurrent ^ codes[lowerBound]);

	// do not reset delta
	tmp = 0;

	do
	{
		delta = (delta + 1) >> 1;

		if (leadingZero <
			leadingZeroRel(codeCurrent ^ codes[index + (tmp + delta) * direction]))
			tmp += delta;
	} while (1 < delta);

	// TODO: remove min and multiplication
	uint location = index + tmp * direction + min(direction, 0);

	uint2 bounds;

	if (min(index, leadingZero) == location)
		bounds.x = location + numObjects;
	else
		bounds.x = location;

	if (max(index, leadingZero) == location + 1)
		bounds.y = location + 1 + numObjects;
	else
		bounds.y = location + 1;

	return bounds;
}

/*
Calculate where to split the tree.
*/

uint calcSlice(uint2 bounds)
{
	uint codeL = codes[bounds.x],
		codeR = codes[bounds.y];

	// check if the codes are the same
	if (codeL == codeR)
		return (codeL + codeR) >> 1;

	// get relative leading zeros
	uint leadingZero = leadingZeroRel(codeL ^ codeR);

	// binary search

	uint slice = bounds.x;
	uint delta = bounds.y - bounds.x;

	uint approxSlice;

	do
	{
		delta = (delta + 1) >> 1;
		approxSlice = slice + delta;

		if (approxSlice < bounds.y
			&& leadingZero < leadingZeroRel(codeL ^ codes[approxSlice]))
			slice = approxSlice;
	} while (1 < delta);

	return slice;
}

[numthreads(NUM_THREADS, 1, 1)]
void main(uint threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	// load in the leaf nodes (load factor of 2)
	// TODO: add this to radix sort
	for (uint loadi = 0; loadi < 2; loadi++)
		nodes[threadID * 2 + loadi].index = sortedIndex[threadID * 2 + loadi];

	DeviceMemoryBarrierWithGroupSync();

	uint index = threadID.x;

	// construct the tree
	uint2 range = calcRange(index);

	uint slice = calcSlice(range);

	uint indexL = (slice == range.x) ? slice : slice + numObjects,
		indexR = (slice + 1 == range.y) ? slice + 1 : slice + 1 + numObjects;

	// set the children

	nodes[index].childL = indexL;
	nodes[index].childR = indexR;

	// set the parent

	nodes[indexL].parent = index;
	nodes[indexR].parent = index;
}