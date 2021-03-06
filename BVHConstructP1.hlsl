#include <RayTraceGlobal.hlsl>

#define LPR_ERR -1

struct LeadingPrefixRet
{
	int num;
	uint index;
};

/*
BVH Construction is based on:
http://devblogs.nvidia.com/parallelforall/wp-content/uploads/2012/11/karras2012hpg_paper.pdf
*/

/*
Since BVH construction only multiplies by +/- 1 (direction), this macro computes the result
of the multiplication using bitwise operations rather than multiplication.
Please note that it is assumed that multiplication takes longer than the below
operations.
*/

#define MULTIPLY_BY_POSNEG(x, s) ((x & ~(s & (s >> 1))) | ((~x + 1) & (s & (s >> 1))))

/*
DeBruijin lookup table.
The table cannot be inlined in the method
because it takes up extra local space.
*/

static int deBruijinLookup[] =
{
	0, 31, 9, 30, 3, 8, 13, 29, 2,
	5, 7, 21, 12, 24, 28, 19,
	1, 10, 4, 14, 6, 22, 25,
	20, 11, 15, 23, 26, 16, 27, 17, 18
};

uint leadingZero(uint data)
{
	if (data == 0)
		return 32;

	// remove non-leading zeros
	data |= data >> 1;
	data |= data >> 2;
	data |= data >> 4;
	data |= data >> 8;
	data |= data >> 16;
	data++;

	return deBruijinLookup[data * 0x076be629 >> 27];
}

/*
Gets number of leeading zeros by representing them
as ones pushed to the right.  Does not give meaningful
number but the relative output is correct.
*/

uint leadingPrefix(uint d1, uint d2, uint index1, uint index2)
{
	uint lpr = leadingZero(d1 ^ d2);

	// use the index as a tie breaker if they are the same
	if (lpr == 32)
	{
		lpr += leadingZero(index1 ^ index2);
	}

	return lpr;
}

/*
Same as leadingPrefix but does bounds checks.
*/

int leadingPrefixBounds(uint d1, uint d1Index, int index)
{
	if (0 <= index && index < (int)numObjects)
		return leadingPrefix(d1, BVHTree[index].code, d1Index, index);
	else
		return LPR_ERR;
}

/*
Check if a < b.
*/

bool lessThanPrefix(int a, int b)
{
	return (a < b);
}

/*
Find the children of the node.
*/

uint2 getChildren(int index)
{
	uint codeCurrent = BVHTree[index].code;

	// get range direction
	int direction = lessThanPrefix(leadingPrefixBounds(codeCurrent, index, index + 1),
		leadingPrefixBounds(codeCurrent, index, index - 1)) ? -1 : 1;

	// get upper bound of length range
	int minLeadingZero =
		leadingPrefixBounds(codeCurrent, index, index - direction);
	uint boundLen = 2;

	for (;
		lessThanPrefix(minLeadingZero, leadingPrefixBounds(
			codeCurrent, index, index + boundLen * direction));
		boundLen <<= 1) {
	}
	
	// find lower bound
	int delta = boundLen;

	int deltaSum = 0;

	do
	{
		delta = (delta + 1) >> 1;

		if (lessThanPrefix(minLeadingZero,
			leadingPrefixBounds(
				codeCurrent, index, index + (deltaSum + delta) * direction)))
			deltaSum += delta;
	} while (1 < delta);

	uint boundStart = index + deltaSum * direction;

	// find slice range
	int leadingZero = leadingPrefixBounds(codeCurrent, index, boundStart);

	delta = deltaSum;
	int tmp = 0;

	do
	{
		delta = (delta + 1) >> 1;

		if (lessThanPrefix(leadingZero,
			leadingPrefixBounds(codeCurrent, index, index + (tmp + delta) * direction)))
			tmp += delta;
	} while (1 < delta);

	uint location = index + tmp * direction + min(direction, 0);

	uint2 children;

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

[numthreads(DATA_SIZE, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	// should only cause the last thread to diverge
	if (threadID.x < numObjects - 1)
	{
		// construct the tree
		uint2 children = getChildren(threadID.x);

		// set the children

		BVHTree[threadID.x + numObjects].childL = children.x;
		BVHTree[threadID.x + numObjects].childR = children.y;

		// set the parent

		BVHTree[children.x].parent = threadID.x + numObjects;
		BVHTree[children.y].parent = threadID.x + numObjects;
	}
	else // set the parent of the root node to -1
		BVHTree[numObjects].parent = UINT_MAX;
}
