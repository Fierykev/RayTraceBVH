#define NUM_THREADS 32

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
	int numObjects;
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

int leadingZeroRel(uint d1, uint d2)
{
	const int deBruijinLookup[] =
	{
		0, 31, 9, 30, 3, 8, 13, 29, 2,
		5, 7, 21, 12, 24, 28, 19,
		1, 10, 4, 14, 6, 22, 25,
		20, 11, 15, 23, 26, 16, 27, 17, 18
	};

	uint data = d1 ^ d2;

	// remove non-leading zeros

	data |= data >> 1;
	data |= data >> 2;
	data |= data >> 4;
	data |= data >> 8;
	data |= data >> 16;
	data++;

	return data ? deBruijinLookup[data * 0x076be629 >> 27] : 32;
}

/*
Same as leadingZeroRel but does bounds checks.
*/

int leadingZeroCode(uint d1, int index)
{
	//return leadingZeroRel(d1, codes[clamp(index, 0, numObjects - 1)]);

	return (0 <= index && index < numObjects) ? leadingZeroRel(d1, nodes[index].code) : -1;
}


/*
Find the children of the node.
*/

int2 getChildren(int index)
{
	uint codeCurrent = nodes[index].code;

	// get range direction
	int direction = sign(leadingZeroCode(codeCurrent, index + 1)
		- leadingZeroCode(codeCurrent, index - 1));

	// get upper bound of length range
	int minLeadingZero = leadingZeroCode(codeCurrent, index - direction);
	uint boundLen = 2;

	// TODO: change back to multiply by 4
	for (;
	minLeadingZero < leadingZeroCode(
		codeCurrent, index + boundLen * direction);
	boundLen <<= 1) {
	}

	// find lower bound
	int delta = boundLen;

	int deltaSum = 0;

	do
	{
		delta = (delta + 1) >> 1;

		if (minLeadingZero <
			leadingZeroCode(
				codeCurrent, index + (deltaSum + delta) * direction))
			deltaSum += delta;
	} while (1 < delta);

	int boundStart = index + deltaSum * direction;

	//return int2(lowerBound, upperBound);

	// find slice range
	int leadingZero = leadingZeroCode(codeCurrent, boundStart);

	delta = deltaSum;
	int tmp = 0;

	do
	{
		delta = (delta + 1) >> 1;

		if (leadingZero <
			leadingZeroCode(codeCurrent, index + (tmp + delta) * direction))
			tmp += delta;
	} while (1 < delta);

	// TODO: remove min and multiplication
	int location = index + tmp * direction + min(direction, 0);

	int2 children;

	if (min(index, boundStart) == location)
		children.x = location;
	else
		children.x = location + numObjects;

	if (max(index, boundStart) == location + 1)
		children.y = location + 1;
	else
		children.y = location + 1 + numObjects;

	return children;
}

[numthreads(NUM_THREADS, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	// load in the leaf nodes (load factor of 2)
	// TODO: add this to radix sort
	for (uint loadi = 0; loadi < 2; loadi++)
		nodes[threadID.x * 2 + loadi].code = sortedIndex[threadID.x * 2 + loadi];

	DeviceMemoryBarrierWithGroupSync();

	// construct the tree
	int2 children = getChildren(threadID.x);

	// set the children

	nodes[threadID.x + numObjects].childL = children.x;
	nodes[threadID.x + numObjects].childR = children.y;

	// set the parent

	nodes[children.x].parent = threadID.x + numObjects;
	nodes[children.y].parent = threadID.x + numObjects;
}