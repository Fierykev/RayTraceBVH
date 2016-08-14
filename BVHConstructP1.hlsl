#include <RadixSortGlobal.hlsl>
#include <ErrorGlobal.hlsl>

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

/*
Gets number of leeading zeros by representing them
as ones pushed to the right.  Does not give meaningful
number but the relative output is correct.
*/

int leadingPrefix(uint d1, uint d2, uint index1, uint index2)
{
	// use the index as a tie breaker if they are the same
	uint data = d1 == d2 ? index1 ^ index2 : d1 ^ d2;

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
Same as leadingPrefix but does bounds checks.
*/

int leadingPrefixBounds(uint d1, uint d1Index, int index)
{
	return (0 <= index && index < (int)numObjects) ? leadingPrefix(d1, BVHTree[index].code, d1Index, index) : -1;
}

/*
Find the children of the node.
*/

int2 getChildren(int index)
{
	uint codeCurrent = BVHTree[index].code;

	// get range direction
	int direction = sign(leadingPrefixBounds(codeCurrent, index, index + 1)
		- leadingPrefixBounds(codeCurrent, index, index - 1));

	// get upper bound of length range
	int minLeadingZero = leadingPrefixBounds(codeCurrent, index, index - direction);
	uint boundLen = 2;

	// TODO: change back to multiply by 4

	for (;
	minLeadingZero < leadingPrefixBounds(
		codeCurrent, index, index + MULTIPLY_BY_POSNEG(boundLen, direction));
	boundLen <<= 1) {
	}

	// find lower bound
	int delta = boundLen;

	int deltaSum = 0;

	do
	{
		delta = (delta + 1) >> 1;

		if (minLeadingZero <
			leadingPrefixBounds(
				codeCurrent, index, index + MULTIPLY_BY_POSNEG((deltaSum + delta), direction)))
			deltaSum += delta;
	} while (1 < delta);

	int boundStart = index + MULTIPLY_BY_POSNEG(deltaSum, direction);

	// find slice range
	int leadingZero = leadingPrefixBounds(codeCurrent, index, boundStart);

	delta = deltaSum;
	int tmp = 0;

	do
	{
		delta = (delta + 1) >> 1;

		if (leadingZero <
			leadingPrefixBounds(codeCurrent, index, index + MULTIPLY_BY_POSNEG((tmp + delta), direction)))
			tmp += delta;
	} while (1 < delta);

	int location = index + MULTIPLY_BY_POSNEG(tmp, direction) + min(direction, 0);

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

[numthreads(DATA_SIZE, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	// should only cause the last thread to diverge
	if (threadID.x < numObjects - 1)
	{
		// construct the tree
		int2 children = getChildren(threadID.x);

		// set the children

		BVHTree[threadID.x + numObjects].childL = children.x;
		BVHTree[threadID.x + numObjects].childR = children.y;

		// set the parent

		BVHTree[children.x].parent = threadID.x + numObjects;
		BVHTree[children.y].parent = threadID.x + numObjects;
	}
	else // set the parent of the root node to -1
		BVHTree[numObjects].parent = -1;
}