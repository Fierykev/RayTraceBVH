#include <stdio.h>
#include <iostream>
#include <bitset>
#include <algorithm>
#include <iomanip>
#include "TestData.h"

#include "SaveBMP.h"

using namespace std;

#define DATA_SIZE 256

#define NUM_GRPS 23

#define MULTIPLY_BY_POSNEG(x, s) ((x & ~(s & (s >> 1))) | ((~x + 1) & (s & (s >> 1))))

#define EPSILON .00001

#define STACK_SIZE 2<<5

const uint numObjects = DATA_SIZE * NUM_GRPS;

DebugNode BVHTree[numObjects * 2];

DebugNode sortedCodes[DATA_SIZE * NUM_GRPS], backbufferCodes[DATA_SIZE * NUM_GRPS], codeData[NUM_GRPS][DATA_SIZE];

// gen data
uint positionNotPresent[NUM_GRPS][DATA_SIZE], positionPresent[NUM_GRPS][DATA_SIZE];

uint numOnesBuffer[NUM_GRPS];

uint transferBuffer[DATA_SIZE * NUM_GRPS];

VERTEX* verts;
XMMATRIX* world;
XMMATRIX* worldViewProjection;
uint* indices;
uint numIndices;

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
// size of number of objects - 1 = internal BVHTree

int sign(int data)
{
	if (data < 0)
		return -1;

	return 1;
}

/*
Update the vert and pass back a new vert.
*/

VERTEX getUpdateVerts(uint index)
{
	VERTEX updateVert;
	updateVert.position = mul(float4(verts[index].position, 1),
		worldViewProjection);
	updateVert.normal = mul(float4(verts[index].normal, 1),
		world);
	updateVert.texcoord = verts[index].texcoord;

	return updateVert;
}

/*
Find the intersection between a ray and a triangle.
*/

float rayTriangleCollision(Ray ray, Triangle tri)
{
	float3 edge1 = tri.verts[1].position - tri.verts[0].position;
	float3 edge2 = tri.verts[2].position - tri.verts[0].position;

	float3 tmpVar = cross(ray.direction, edge2);
	float dx = dot(edge1, tmpVar);

	// no determinant
	if (abs(dx) < EPSILON)
		return -1.f;

	// calc the inverse determinant
	float idx = 1.f / dx;

	float3 rayToTri = ray.origin - tri.verts[0].position;

	// compute the inverse of the 3x3 matrix

	float3 inverse;

	// compute inverse x
	inverse.x = dot(rayToTri, tmpVar) * idx;

	if (inverse.x < .0f || 1.f < inverse.x)
		return -1.f;

	// update with cross product
	tmpVar = cross(rayToTri, edge1);

	// compute inverse y
	inverse.y = dot(ray.direction, tmpVar) * idx;

	// intersection is not within the triangle bounds
	if (inverse.y < .0f || 1.f < inverse.x + inverse.y)
		return -1.f;

	// calculate inverse z (distance)
	inverse.z = dot(edge2, tmpVar) * idx;

	// ray intersects the triangle
	if (EPSILON < inverse.z)
		return inverse.z;

	return -1.f;
}

/*
The slab method is used for collision.
*/

bool rayBoxCollision(Ray ray, Box box)
{
	float3 deltaMin = (box.bbMin - ray.origin) * ray.invDirection;
	float3 deltaMax = (box.bbMax - ray.origin) * ray.invDirection;

	float3 minVector = min(deltaMin, deltaMax);
	float3 maxVector = max(deltaMin, deltaMax);

	float minVal = max(max(minVector.x, minVector.y), minVector.z);
	float maxVal = min(min(maxVector.x, maxVector.y), maxVector.z);

	return 0 <= maxVal && minVal <= maxVal;
}

void findCollision(Ray ray, ColTri& colTri)
{
	// set no collision
	colTri.collision = false;
	colTri.distance = 0;

	// create the stack vars (1 item is unusable in the stack
	// for controle flow reasons)
	int stackIndex = 0;
	int stack[STACK_SIZE];

	stack[0] = -1;

	// start from the root
	int nodeID = numObjects;

	// tmp vars
	int leftChildNode, rightChildNode;

	bool leftChildCollision, rightChildCollision;
	int leftLeaf, rightLeaf;

	Triangle triTmp;
	float distance;

	int index;

	do
	{
		// record the index of the children
		leftChildNode = BVHTree[nodeID].childL;
		rightChildNode = BVHTree[nodeID].childR;

		// check for leaf node
		if (leftChildNode == -1 && rightChildNode == -1)
		{
			// get the index
			index = BVHTree[nodeID].index;

			// get triangle
			for (uint i = 0; i < 3; i++)
				triTmp.verts[i] = getUpdateVerts(indices[index + i]);

			// run collission test
			distance = rayTriangleCollision(ray, triTmp);

			//  update the triangle
			if (distance != -1 && (!colTri.collision || distance < colTri.distance))
			{
				// set collision to true
				colTri.collision = true;

				// store the vertex
				colTri.tri = triTmp;
				colTri.distance = distance;
			}

			// remove the stack top
			nodeID = stack[stackIndex--];

			continue;
		}

		// check for collisions with children
		leftChildCollision = rayBoxCollision(ray, BVHTree[leftChildNode].bbox);
		rightChildCollision = rayBoxCollision(ray, BVHTree[rightChildNode].bbox);

		if (!leftChildCollision && !rightChildCollision)
		{
			// remove the stack top
			nodeID = stack[stackIndex--];
		}
		else
		{
			// store the right node on the stack if both right and left
			// boxes intersect the ray
			if (leftChildCollision && rightChildCollision)
				stack[++stackIndex] = rightChildNode;

			// update the nodeID depending on if the left side intersected
			nodeID = leftChildCollision ? leftChildNode : rightChildNode;
		}
	} while (stackIndex != -1);
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

LeadingPrefixRet leadingPrefix(uint d1, uint d2, uint index1, uint index2)
{
	// use the index as a tie breaker if they are the same
	uint data = index1 ^ index2;

	// remove non-leading zeros

	data |= data >> 1;
	data |= data >> 2;
	data |= data >> 4;
	data |= data >> 8;
	data |= data >> 16;
	data++;

	LeadingPrefixRet lpr;

	lpr.num = data ? deBruijinLookup[data * 0x076be629 >> 27] : 32;
	lpr.index = index2;

	return lpr;
}

/*
Same as leadingPrefix but does bounds checks.
*/

LeadingPrefixRet leadingPrefixBounds(uint d1, uint d1Index, int index)
{
	LeadingPrefixRet lprERR;
	lprERR.num = -1;

	//return leadingPrefix(d1, codes[clamp(index, 0, numObjects - 1)]);
	// TODO: remove branch
	return (0 <= index && index < numObjects) ? leadingPrefix(d1, BVHTree[index].code, d1Index, index) : lprERR;
}

/*
Check if a < b.
*/

bool lessThanPrefix(LeadingPrefixRet a, LeadingPrefixRet b)
{
	return (a.num < b.num) || (a.index == b.index && a.index < b.index);
}

/*
Find the children of the node.
*/

int2 getChildren(int index)
{
	uint codeCurrent = BVHTree[index].code;

	// get range direction
	int direction = sign(leadingPrefixBounds(codeCurrent, index, index + 1).num
		- leadingPrefixBounds(codeCurrent, index, index - 1).num);

	// get upper bound of length range
	LeadingPrefixRet minLeadingZero = leadingPrefixBounds(
		codeCurrent, index, index - direction);

	uint boundLen = 2;

	// TODO: change back to multiply by 4
	for (;
	lessThanPrefix(minLeadingZero, leadingPrefixBounds(
		codeCurrent, index, index + MULTIPLY_BY_POSNEG(boundLen, direction)));
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
				codeCurrent, index, index + MULTIPLY_BY_POSNEG((deltaSum + delta), direction))))
			deltaSum += delta;
	} while (1 < delta);

	int boundStart = index + MULTIPLY_BY_POSNEG(deltaSum, direction);

	//return int2(lowerBound, upperBound);

	// find slice range
	LeadingPrefixRet leadingZero = leadingPrefixBounds(codeCurrent, index, boundStart);

	delta = deltaSum;
	int tmp = 0;

	do
	{
		delta = (delta + 1) >> 1;

		if (lessThanPrefix(leadingZero,
			leadingPrefixBounds(codeCurrent, index, index + MULTIPLY_BY_POSNEG((tmp + delta), direction))))
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

void printPostOrder(DebugNode* p, int indent = 0)
{
	if (p->childL != -1) printPostOrder(&BVHTree[p->childL], indent + 4);
	if (p->childR != -1) printPostOrder(&BVHTree[p->childR], indent + 4);
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

float3 getPosTrans(uint index)
{
	return (float3)mul(float4(verts[index].position, 1), worldViewProjection);
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

uint calcMortonCode(float3 p)
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
		code[i] = (uint)p[i];// bitTwiddling((uint)p[i]);
	}

	// combine codes for the output
	return code[2] | code[1] << 1 | code[0] << 2;
}

uint rand(uint lfsr)
{
	//uint bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
	return ((uint)(lfsr / 65536) % 32768); //lfsr = (lfsr >> 1) | (bit << 15);
}

DebugNode* constructDebugTree(VERTEX* pverts, uint* pindices, uint pnumIndices,
	XMMATRIX* pworld, XMMATRIX* pworldViewProjection,
	uint screenWidth, uint screenHeight)
{
	verts = pverts;
	indices = pindices;
	numIndices = pnumIndices;
	world = pworld;
	worldViewProjection = pworldViewProjection;

	// radix sort

	float3 bbMin, bbMax;
	float3 avg;

	float3 sceneBBMax = float3(36, 42, 44);
	float3 sceneBBMin = float3(-51, -2, -43);

	for (uint i = 0; i < DATA_SIZE; i++)
	{
		for (uint j = 0; j < NUM_GRPS; j++)
		{
			uint threadID = j * DATA_SIZE + i;

			uint indicesBase = threadID * 3;

			uint index = threadID;

			bbMin = float3(0, 0, 0);
			bbMax = float3(0, 0, 0);
			avg = float3(0, 0, 0);

			BVHTree[index].code = 0;

			if (indicesBase < numIndices)
			{
				// bounding box computation	
				float3 vertData = getPosTrans(indices[indicesBase]);

				// set the start value
				bbMin = bbMax = avg = vertData;

				// load in the data (3 indices and 3 verts)
				for (uint sumi = 1; sumi < 3; sumi++)
				{
					vertData = getPosTrans(indices[indicesBase + sumi]);

					bbMin = min(bbMin, vertData);
					bbMax = max(bbMax, vertData);

					avg += vertData;
				}

				// divide by three to compute the average
				avg /= 3.f;

				// store the code
				BVHTree[index].code = rand(index);// calcMortonCode((avg - sceneBBMin) / (sceneBBMax - sceneBBMin));
			}

			// store bounding box info
			BVHTree[index].bbox.bbMin = float3(rand(index), rand(index % 2), rand(index % 7));// bbMin;
			BVHTree[index].bbox.bbMax = float3(rand(index / 3), rand(index % 3), rand(index / 7));// bbMax;

			// set children to invalid
			BVHTree[index].childL = -1;
			BVHTree[index].childR = -1;

			// store vert index data
			BVHTree[index].index = indicesBase;

			// copy over to soring array
			codeData[j][i] = BVHTree[index];

			/*
			float pointData = (i + 1) * (j + 1);

			float3 point = float3(
				rand(pointData) / rand(rand(pointData)),
				rand(rand(pointData)) / rand(pointData),
				rand(rand(rand(pointData))) / rand(pointData)
				);


			// randomly set the bounding box
			//codeData[j][i].bbox.bbMin = point;
			//codeData[j][i].bbox.bbMax = float3(point.y, point.z, point.x);

			codeData[j][i].code = calcMortonCode((float*)&point); //rand();
			*/
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
					positionNotPresent[j][groupThreadID * 2 + loadi] = !(codeData[j][groupThreadID * 2 + loadi].code & (1 << radixi));
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
						((codeData[j][groupThreadID * 2 + loadi].code & (1 << radixi)) ?
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
					sortedCodes[positionPresent[j][index]].code = j * DATA_SIZE + index;// codeData[j][index];
				}
			}
		}

		for (uint i = 0; i < NUM_GRPS * DATA_SIZE; i++)
		{
			uint x0 = (sortedCodes[i].code) / DATA_SIZE, y0 = (sortedCodes[i].code) % DATA_SIZE;
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
		if ((codeData[x0][y0].code) > (codeData[x1][y1].code))
			printf("ERR\n");
	}
	
	// bvh gen

	for (uint i = 0; i < numObjects; i++)
	{
		uint x0 = i / DATA_SIZE, y0 = i % DATA_SIZE;

		BVHTree[i] = codeData[x0][y0];
	}

	for (uint i = 1; i < numObjects; i++)
	{
		if ((BVHTree[i - 1].code) > (BVHTree[i].code))
			printf("ERR\n");
	}
	
	for (uint i = numObjects; i < numObjects * 2 - 1; i++)
	{
		BVHTree[i].code = i;
	}

	for (int threadID = 0; threadID < numObjects - 1; threadID++)
	{
		int index = threadID;

		// construct the tree
		int2 children = getChildren(index);

		// set the children

		BVHTree[index + numObjects].childL = children.x;
		BVHTree[index + numObjects].childR = children.y;

		// set the parent

		BVHTree[children.x].parent = index + numObjects;
		BVHTree[children.y].parent = index + numObjects;
	}

	// output tree
	//printPostOrder(&BVHTree[numObjects]);

	// set root to -1
	BVHTree[numObjects].parent = -1;
	
	// compute the bounding box

	// reset num ones buffer
	for (uint i = 0; i < DATA_SIZE * NUM_GRPS; i++)
		transferBuffer[i] = 0;

	for (uint threadID = 0; threadID < DATA_SIZE * NUM_GRPS; threadID++)
	{
		int nodeID = BVHTree[threadID].parent;

		uint value;
		
		// atomically add to prevent two threads from entering a node
		InterlockedAdd(transferBuffer[nodeID - numObjects], 1, value);

		while (value)
		{
			// compute the union of the bounding boxes
			BVHTree[nodeID].bbox.bbMin = min(BVHTree[BVHTree[nodeID].childL].bbox.bbMin,
				BVHTree[BVHTree[nodeID].childR].bbox.bbMin);

			BVHTree[nodeID].bbox.bbMax = max(BVHTree[BVHTree[nodeID].childL].bbox.bbMax,
				BVHTree[BVHTree[nodeID].childR].bbox.bbMax);

			// get the parent
			nodeID = BVHTree[nodeID].parent;

			// atomically add to prevent two threads from entering a node
			if (nodeID != -1)
				InterlockedAdd(transferBuffer[nodeID - numObjects], 1, value);
			else
				break;
		}
	}

	return BVHTree;

	// run ray traversal
	Ray ray;
	ColTri colTri;

	uint halfWidth = screenWidth >> 1;
	uint halfHeight = screenHeight >> 1;

	uint gloablIndexID;

	char3* outputTex = new char3[screenWidth * screenHeight];

	for (uint threadIDx = 0;
	threadIDx < screenWidth; threadIDx++)
	{
		for (uint threadIDy = 0;
		threadIDy < screenHeight; threadIDy++)
		{
			// get global index ID
			gloablIndexID = threadIDy * screenWidth + threadIDx;

			// get the ray position
			// get the ray positioon
			ray.origin = float3(((float)threadIDx - halfWidth),
				((float)threadIDy - halfHeight), 0);

			// get the ray direction
			ray.direction = float3(0, 0, 1);

			// get the inverse of the direction
			ray.invDirection = 1.f / ray.direction;

			// find the collision with a triangle
			findCollision(ray, colTri);
			
			if (colTri.collision)
			{
				outputTex[gloablIndexID] = char3(colTri.distance,
					colTri.distance,
					colTri.distance);
			}
			else
			{
				outputTex[gloablIndexID] = char3(255, 0, 0);
			}
		}
	}

	//SaveBMP((char*)outputTex, screenWidth, screenHeight, sizeof(char3) * screenWidth * screenHeight);

	//system("PAUSE");

	//exit(0);

	return BVHTree;
}