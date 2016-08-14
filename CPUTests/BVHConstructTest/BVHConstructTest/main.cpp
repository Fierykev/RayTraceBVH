#include <stdio.h>
#include <iostream>
#include <bitset>
#include <algorithm>
#include <iomanip>

typedef unsigned int uint;

using namespace std;

#define NUM_THREADS 8
#define DATA_SIZE 8

#define NUM_GRPS 1

#define MULTIPLY_BY_POSNEG(x, s) ((x & ~(s & (s >> 1))) | ((~x + 1) & (s & (s >> 1))))

struct int2
{
	int x, y;

	int2(int px, int py)
	{
		x = px;
		y = py;
	}

	int2()
	{

	}
};

struct float4
{
	float w, x, y, z;
	// TODO: CHECK ORDER
	float4(float pw, float px, float py, float pz)
	{
		w = pw;
		x = px;
		y = py;
		z = pz;
	}

	float4()
	{

	}
};

struct NODE
{
	int parent;
	int childL, childR;

	uint code;
};

const uint numObjects = DATA_SIZE * NUM_GRPS;

NODE nodes[numObjects * 2 + 1];
//uint codes[numObjects];

int any(uint data)
{
	if (data == 0)
		return 1;
	
	return 0;
}

int clamp(int x, int min, int max)
{
	if (x < min)
		return min;
	else if (x > max)
		return max;

	return x;
}

// size of number of objects = leaf node
// size of number of objects - 1 = internal nodes

int sign(int data)
{
	if (data < 0)
		return -1;

	return 1;
}

static const int deBruijinLookup[] =
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
	//return leadingPrefix(d1, codes[clamp(index, 0, numObjects - 1)]);
	// TODO: remove branch
	return (0 <= index && index < numObjects) ? leadingPrefix(d1, nodes[index].code, d1Index, index) : -1;
}

/*
Find the children of the node.
*/

int2 getChildren(int index)
{
	uint codeCurrent = nodes[index].code;

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
	
	//return int2(lowerBound, upperBound);
	
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

	// TODO: remove min and multiplication
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

/*
Calculate where to split the tree.
*/
/*
uint calcSlice(int2 bounds)
{
	uint codeL = codes[bounds.x],
		codeR = codes[bounds.y];

	// check if the codes are the same
	if (codeL == codeR)
		return (codeL + codeR) >> 1;

	// get relative leading zeros
	int leadingZero = leadingPrefix(codeL, codeR);

	// binary search

	int slice = bounds.x;
	int delta = bounds.y - bounds.x;

	int approxSlice;

	do
	{
		delta = (delta + 1) >> 1;
		approxSlice = slice + delta;

		if (approxSlice < bounds.y
			&& leadingZero < leadingPrefixBounds(codeL, approxSlice))
			slice = approxSlice;
	} while (1 < delta);

	return slice;
}*/

void printPostOrder(NODE* p, int indent = 0)
{
	if (p->childL != -1) printPostOrder(&nodes[p->childL], indent + 4);
	if (p->childR != -1) printPostOrder(&nodes[p->childR], indent + 4);
	if (indent) {
		std::cout << std::setw(indent) << ' ';
	}
	cout << p->code << "\n ";
}

void main()
{
	uint data[numObjects] = {
	
		0b00001, 0b00010, 0b00100,
		0b00101, 0b10011, 0b11000,
		0b11001, 0b11110
	
	};

	for (uint i = 0; i < numObjects; i++)
	{
		nodes[i].code = data[i];
		nodes[i].childL = -1;
		nodes[i].childR = -1;
	}

	for (uint i = numObjects; i < numObjects * 2 - 1; i++)
	{
		nodes[i].code = i;
	}

	for (int threadID = 0; threadID < numObjects - 1; threadID++)
	{
		// load in the leaf nodes (load factor of 2)
		// TODO: add this to radix sort

		int index = threadID;

		// construct the tree
		int2 children = getChildren(index);

		//int slice = calcSlice(range);

		//int indexL = (slice == range.x) ? slice : slice + numObjects,
			//indexR = (slice + 1 == range.y) ? slice + 1 : slice + 1 + numObjects;

		// set the children
		
		nodes[index + numObjects].childL = children.x;
		nodes[index + numObjects].childR = children.y;

		// set the parent

		nodes[children.x].parent = index + numObjects;
		nodes[children.y].parent = index + numObjects;
	}

	// output tree
	printPostOrder(&nodes[numObjects]);

	system("PAUSE");
}