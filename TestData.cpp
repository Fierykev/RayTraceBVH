#include <stdio.h>
#include <iostream>
#include <bitset>
#include <algorithm>
#include <iomanip>
#include "TestData.h"

using namespace std;

#define DATA_SIZE 256

#define NUM_GRPS 23

#define MULTIPLY_BY_POSNEG(x, s) ((x & ~(s & (s >> 1))) | ((~x + 1) & (s & (s >> 1))))

const uint numObjects = DATA_SIZE * NUM_GRPS;

NODE nodes[numObjects * 2 - 1];

uint sortedCodes[DATA_SIZE * NUM_GRPS], backbufferCodes[DATA_SIZE * NUM_GRPS], codeData[NUM_GRPS][DATA_SIZE];

// gen data
uint positionNotPresent[NUM_GRPS][DATA_SIZE], positionPresent[NUM_GRPS][DATA_SIZE];

uint numOnesBuffer[NUM_GRPS];

uint transferBuffer[DATA_SIZE * NUM_GRPS];

float3 min(float3 data1, float3 data2)
{
	return float3(min(data1.x, data2.x),
		min(data1.y, data2.y), min(data1.z, data2.z));
}

float3 max(float3 data1, float3 data2)
{
	return float3(max(data1.x, data2.x),
		max(data1.y, data2.y), max(data1.z, data2.z));
}

void InterlockedAdd(uint &dest, uint value, uint& output)
{
	uint origVal = dest;
	dest += value;

	output = origVal;
}

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
	//cout << (data * 0x076be629) << endl;
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

void printPostOrder(NODE* p, int indent = 0)
{
	if (p->childL != -1) printPostOrder(&nodes[p->childL], indent + 4);
	if (p->childR != -1) printPostOrder(&nodes[p->childR], indent + 4);
	if (indent) {
		std::cout << std::setw(indent) << ' ';
	}

	cout << p->code << "\n ";
}

void prefixSum(uint *data)
{
	for (uint upSweepi = 1; upSweepi < DATA_SIZE; upSweepi <<= 1)
	{
		for (uint ID = 0; ID < DATA_SIZE >> 1; ID++)
		{
			uint indexL = ID * (upSweepi << 1) + upSweepi - 1;
			uint indexR = ID * (upSweepi << 1) + (upSweepi << 1) - 1;

			if (indexR < DATA_SIZE)
			{
				data[indexR] += data[indexL];
			}
		}
	}

	data[DATA_SIZE - 1] = 0;

	for (uint downSweepi = DATA_SIZE >> 1; 0 < downSweepi; downSweepi >>= 1)
	{
		for (uint ID = 0; ID < DATA_SIZE >> 1; ID++)
		{
			uint ID_index = ID * (downSweepi << 1);

			uint index_1 = ID_index + downSweepi - 1;
			uint index_2 = ID_index + (downSweepi << 1) - 1;

			if (index_2 < DATA_SIZE)
			{
				uint tmp = data[ID_index + downSweepi - 1];
				data[index_1] = data[index_2];
				data[index_2] = tmp + data[index_2];
			}
		}
	}
}

/**************************************
MORTON CODES
**************************************/

uint bitTwiddling(uint var)
{
	const uint byteMasks[] = { 0x09249249, 0x030c30c3, 0x0300f00f, 0x030000ff, 0x000003ff };
	uint curVal = 16;

	//[unroll(3)]
	for (uint i = 4; 0 < i; i--)
	{
		var &= byteMasks[i]; // set everything other than the last 10 bits to zero
		var |= var << curVal; // add in the byte offset

		curVal >>= 1; // divide by two
	}

	// add in the final mask
	var &= byteMasks[0];

	return var;
}

uint calcMortonCode(float p[3])
{
	uint code[3];

	// multiply out
	for (int i = 0; i < 3; i++)
	{
		// change to base .5
		p[i] *= 1024.f;

		// check if too low
		if (p[i] < 0.f)
			p[i] = 0.f;

		// check if too high		
		if (p[i] >= 1024.f)
			p[i] = 1023.f;
		// TODO FIX CAST
		code[i] = bitTwiddling(p[i]);
	}

	// combine codes for the output
	return code[2] | code[1] << 1 | code[0] << 2;
}

uint rand(uint lfsr)
{
	uint bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
	return lfsr = (lfsr >> 1) | (bit << 15);
}

NODE* constructDebugTree()
{
	// radix sort

	uint sum = 0;

	for (uint i = 0; i < DATA_SIZE; i++)
	{
		for (uint j = 0; j < NUM_GRPS; j++)
		{
			float pointData = (i + 1) * (j + 1);

			float3 point = float3(
				rand(pointData) / rand(rand(pointData)),
				rand(rand(pointData)) / rand(pointData),
				rand(rand(rand(pointData))) / rand(pointData)
				);

			// randomly set the bounding box
			nodes[i].bbMin = point;
			nodes[i].bbMax = float3(point.y, point.z, point.x);

			codeData[j][i] = calcMortonCode((float*)&point); //rand();

			sum += !(codeData[j][i] & (1 << 2));
		}
	}

	for (uint radixi = 0; radixi < 32; radixi++)
	{
		for (uint loadi = 0; loadi < 2; loadi++)
		{
			for (uint j = 0; j < NUM_GRPS; j++)
			{
				for (uint groupThreadID = 0; groupThreadID < DATA_SIZE / 2; groupThreadID++)
				{
					// store the inverted version of the important bit
					positionNotPresent[j][groupThreadID * 2 + loadi] = !(codeData[j][groupThreadID * 2 + loadi] & (1 << radixi));
				}
			}
		}

		// set the one's count
		uint totalOnes[NUM_GRPS];

		for (uint j = 0; j < NUM_GRPS; j++)
			totalOnes[j] = positionNotPresent[j][DATA_SIZE - 1];

		// run a prefix sum
		for (uint j = 0; j < NUM_GRPS; j++)
			prefixSum(positionNotPresent[j]);

		// add the number of ones in this group to a global buffer
		for (uint j = 0; j < NUM_GRPS; j++)
		{
			numOnesBuffer[j] = (totalOnes[j] += positionNotPresent[j][DATA_SIZE - 1]);
		}

		uint netOnes[NUM_GRPS], numPrecOnes[NUM_GRPS];

		for (uint j = 0; j < NUM_GRPS; j++)
		{
			// init to zero
			netOnes[j] = 0;

			// sum up the preceding ones
			for (uint sumi = 0; sumi < NUM_GRPS; sumi++)
			{
				if (sumi == j)
					numPrecOnes[j] = netOnes[j];

				netOnes[j] += numOnesBuffer[sumi];
			}
		}

		// calculate position if the bit is not present
		for (uint loadi = 0; loadi < 2; loadi++)
		{
			for (uint j = 0; j < NUM_GRPS; j++)
			{
				for (uint groupThreadID = 0; groupThreadID < DATA_SIZE / 2; groupThreadID++)
				{
					int index = groupThreadID * 2 + loadi;

					positionPresent[j][index] =
						index + j * DATA_SIZE
						- positionNotPresent[j][index] - numPrecOnes[j]
						+ netOnes[j];
				}
			}
		}

		// get the new positions (re-use positionPresent)
		for (uint loadi = 0; loadi < 2; loadi++)
		{
			for (uint j = 0; j < NUM_GRPS; j++)
			{
				for (uint groupThreadID = 0; groupThreadID < DATA_SIZE / 2; groupThreadID++)
				{
					int index = groupThreadID * 2 + loadi;

					positionPresent[j][index] =
						((codeData[j][groupThreadID * 2 + loadi] & (1 << radixi)) ?
							positionPresent[j][index] :
							positionNotPresent[j][index] + numPrecOnes[j]);
				}
			}
		}

		// write the data to the out buffer
		for (uint loadi = 0; loadi < 2; loadi++)
		{
			for (uint j = 0; j < NUM_GRPS; j++)
			{
				for (uint groupThreadID = 0; groupThreadID < DATA_SIZE / 2; groupThreadID++)
				{
					int index = groupThreadID * 2 + loadi;

					//for (uint indexi = 0; indexi < 3; indexi++)
					//	indexData[positionPresent[index] * 3 + indexi] =
					//		indexData[(groupThreadID.x + loadi) * 3 * 2 + loadi];
					//positionPresent[index]
					sortedCodes[positionPresent[j][index]] = j * DATA_SIZE + index;// codeData[j][index];
				}
			}
		}

		for (uint i = 0; i < NUM_GRPS * DATA_SIZE; i++)
		{
			uint x0 = (sortedCodes[i]) / DATA_SIZE, y0 = (sortedCodes[i]) % DATA_SIZE;
			backbufferCodes[i] = codeData[x0][y0];
		}

		// update codes
		for (uint x = 0; x < DATA_SIZE; x++)
		{
			for (uint y = 0; y < NUM_GRPS; y++)
			{
				codeData[y][x] = backbufferCodes[y * DATA_SIZE + x];
			}
		}
	}

	for (uint i = 1; i < NUM_GRPS * DATA_SIZE; i++)
	{
		uint x0 = (i - 1) / DATA_SIZE, y0 = (i - 1) % DATA_SIZE,
			x1 = i / DATA_SIZE, y1 = i % DATA_SIZE;

		//if ((codeData[x0][y0] & (1 << radixi))  > (codeData[x1][y1] & (1 << radixi))) //if ((sortedCodes[i - 1] & (1 << radixi)) > (sortedCodes[i] & (1 << radixi)))
		if ((codeData[x0][y0]) >(codeData[x1][y1]))
			printf("ERR\n");
	}

	// bvh gen

	for (uint i = 0; i < numObjects; i++)
	{
		uint x0 = i / DATA_SIZE, y0 = i % DATA_SIZE;

		nodes[i].code = codeData[x0][y0];
		nodes[i].childL = -1;
		nodes[i].childR = -1;
	}

	for (uint i = numObjects; i < numObjects * 2 - 1; i++)
	{
		nodes[i].code = i;
	}

	uint tmpData[numObjects * 2 - 1] = { 0 };

	for (int threadID = 0; threadID < numObjects - 1; threadID++)
	{
		int index = threadID;

		// construct the tree
		int2 children = getChildren(index);

		// set the children

		nodes[index + numObjects].childL = children.x;
		nodes[index + numObjects].childR = children.y;

		// set the parent

		nodes[children.x].parent = index + numObjects;
		nodes[children.y].parent = index + numObjects;
	}

	// output tree
	//printPostOrder(&nodes[numObjects]);

	// set root to -1
	nodes[numObjects].parent = -1;
	cout << nodes[6391].childR << " " << numObjects + 500 << endl;

	return nodes;

	// compute the bounding box

	// reset num ones buffer
	for (uint i = 0; i < DATA_SIZE * NUM_GRPS; i++)
		transferBuffer[i] = 0;

	uint maximizeLoop = 0;
	uint maxLoop = 0;

	for (uint threadID = 0; threadID < DATA_SIZE * NUM_GRPS; threadID++)
	{
		int nodeID = nodes[threadID].parent;

		uint value;

		// atomically add to prevent two threads from entering a node
		InterlockedAdd(transferBuffer[nodeID - numObjects], 1, value);

		maxLoop = 0;

		while (value)
		{
			// compute the union of the bounding boxes
			nodes[nodeID].bbMin = min(nodes[nodes[nodeID].childL].bbMin,
				nodes[nodes[nodeID].childR].bbMin);

			nodes[nodeID].bbMax = max(nodes[nodes[nodeID].childL].bbMax,
				nodes[nodes[nodeID].childR].bbMax);

			// get the parent
			nodeID = nodes[nodeID].parent;

			// atomically add to prevent two threads from entering a node
			if (nodeID != -1)
				InterlockedAdd(transferBuffer[nodeID - numObjects], 1, value);
			else
				break;

			maxLoop++;
		}

		maximizeLoop = max(maximizeLoop, maxLoop);
	}

	return nodes;
}