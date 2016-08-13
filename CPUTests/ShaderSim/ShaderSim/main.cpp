#include <algorithm>
#include <time.h>
#include <bitset>
#include <iostream>
#include "ObjectFileLoader.h"

#define NUM_THREADS 128
#define NUM_GROUPS 10
#define DATA_SIZE (NUM_THREADS * 2)

float VERT_NUM = 1000;

using namespace std;

typedef unsigned int uint;

struct float3
{
	float x = 0, y = 0, z = 0;

	float3(float xp, float yp, float zp)
	{
		x = xp;
		y = yp;
		z = zp;
	}

	float3()
	{

	}
};

struct float2
{
	float x = 0, y = 0;
};

struct uint3
{
	unsigned int x = 0, y = 0, z = 0;

	uint3(unsigned int xp, unsigned int yp, unsigned int zp)
	{
		x = xp;
		y = yp;
		z = zp;
	}

	uint3()
	{

	}
};

struct VERTEX
{
	float3 position;
	float3 normal;
	float2 texcoord;
};

uint* indices;

VERTEX* verts;

int numOutput;

float3* output;

uint* codes;

float3 reductionData[DATA_SIZE * 2];

uint3 index;

void setVal()
{
	float3 maxVal = float3(0.f, 0.f, 0.f);

	for (int i = 0; i < DATA_SIZE; i++)
	{
		verts[i].position = float3(rand() / 100.f, rand() / 1000.f, rand() / 1000.f);

		maxVal = float3(max(maxVal.x, verts[i].position.x), max(maxVal.y, verts[i].position.y), max(maxVal.z, verts[i].position.z));
	}

	printf("%f, %f, %f\n", maxVal.x, maxVal.y, maxVal.z);
}

void setOutVal()
{
	float3 maxVal = float3(0.f, 0.f, 0.f);

	for (int i = 0; i < numOutput; i++)
	{
		output[i] = float3(rand() / 100.f, rand() / 1000.f, rand() / 1000.f);

		maxVal = float3(max(maxVal.x, output[i].x), max(maxVal.y, output[i].y), max(maxVal.z, output[i].z));
	}

	printf("%f, %f, %f\n", maxVal.x, maxVal.y, maxVal.z);

	system("PAUSE");
}

float3 findMaxVal(float3 p0, float3 p1)
{
	return float3(max(p0.x, p1.x), max(p0.y, p1.y), max(p0.z, p1.z));
}

void runTest(uint3 groupID)
{
	uint numVerts, stride;

	numVerts = VERT_NUM;

	// get a portion of the global memory and add it to shared (load two at a time)

	for (uint thread = 0; thread < NUM_THREADS; thread++)
	{
		for (uint loadi = 0; loadi < 2; loadi++)
		{
			uint index = (groupID.x + thread) * 2 + loadi;

			reductionData[thread * 2 + loadi] = index < numVerts ? verts[index].position : float3(0, 0, 0);
		}
	}

	// start the reduction
	for (uint i = 1; i < DATA_SIZE; i *= 2)
	{
		for (uint thread = 0; thread < NUM_THREADS; thread++)
		{
			uint index = i * 2 * thread;

			if (index < DATA_SIZE)
				reductionData[index] = findMaxVal(reductionData[index], reductionData[index + i]);
		}
	}

	printf("%f, %f, %f\n", reductionData[0].x, reductionData[0].y, reductionData[0].z);
}

void finalReduction(uint3 groupID)
{
	int m = 0;
	uint remVerts = numOutput / DATA_SIZE;
	uint power = 1;
	// finish the reduction by looking at all group outputs
	uint groupOutputNum = ceil(log(numOutput) / log(DATA_SIZE));

	for (uint fLoop = 0; fLoop < groupOutputNum; fLoop++)
	{
		for (uint groupIDT = 0; groupIDT <= groupID.x; groupIDT++)
		{
			// no more need for this group
			if (remVerts < groupIDT)
				continue; // TODO: change to return when putting in shader
			printf("%i, %i, %f\n", ++m, fLoop, floor(remVerts));
			// fill up the groupshared buffer
			for (uint loadf = 0; loadf < 2; loadf++)
			{
				for (uint thread = 0; thread < NUM_THREADS; thread++)
				{
					uint index = (groupIDT * DATA_SIZE + thread * 2 + loadf) * power;

					reductionData[thread * 2 + loadf] = index < numOutput ? output[index] : float3(0, 0, 0);
					//printf("%f, %f, %f\n", reductionData[thread * 2 + loadf].x, reductionData[thread * 2 + loadf].y, reductionData[thread * 2 + loadf].z);
				}
			}

			// continue the reduction
			for (uint fRed = 1; fRed < DATA_SIZE; fRed *= 2)
			{
				for (uint thread = 0; thread < NUM_THREADS; thread++)
				{
					uint index = fRed * 2 * thread;

					if (index < DATA_SIZE)
						reductionData[index] = findMaxVal(reductionData[index], reductionData[index + fRed]);
				}
			}

			// write the data to global

			output[groupIDT * DATA_SIZE * power] = reductionData[0];
			printf("%f, %f, %f\n", output[groupIDT * DATA_SIZE * power].x, output[groupIDT * DATA_SIZE * power].y, output[groupIDT * DATA_SIZE * power].z);
		}
		printf("------------------\n");
		// add in groupID.x == 0
		remVerts /= DATA_SIZE;

		// multiply the power
		power *= DATA_SIZE;
	}

	printf("%f, %f, %f\n", output[0].x, output[0].y, output[0].z);
}

unsigned int expand(unsigned int var)
{
	const unsigned int byteMasks[] = { 0x09249249, 0x030c30c3, 0x0300f00f, 0x030000ff, 0x000003ff };
	int curVal = 16;

	for (int i = 4; 0 < i; i--)
	{
		var &= byteMasks[i]; // set everything other than the last 10 bits to zero
		var |= var << curVal; // add in the byte offset

							  // divide by two
		curVal /= 2;
	}

	var &= byteMasks[0];

	return var;
}

unsigned int calcMorton(float x, float y, float z)
{
	float p[] = { x, y, z };

	unsigned int code[3];

	// multiply out
	for (int i = 0; i < 3; i++)
	{
		// change to base .5
		p[i] *= 1024.f;

		if (p[i] < 0)
			p[i] = 0;
		else if (p[i] >= 1024)
			p[i] = 1023;

		code[i] = expand(p[i]);
	}
	//outputUnsignedIntBits(p[0]);
	//outputUnsignedIntBits(code[0]);
	// combine and return the code
	return code[2] | code[1] << 1 | code[0] << 2;
}

void outputUnsignedIntBits(const unsigned int val)
{
	std::bitset<32> x(val);
	std::cout << x << endl;
}

void main()
{
	ObjLoader obj;
	obj.Load("Obj/Test.obj");

	VERT_NUM = obj.getNumVertices();

	verts = new VERTEX[VERT_NUM];

	indices = new uint[obj.getNumIndices()];

	numOutput = ceil(VERT_NUM / (float)DATA_SIZE + 1);

	output = new float3[numOutput];

	// copy over verts
	for (int i = 0; i < obj.getNumVertices(); i++)
	{
		verts[i].position.x = obj.getVertices()[i].position.x;
		verts[i].position.y = obj.getVertices()[i].position.y;
		verts[i].position.z = obj.getVertices()[i].position.z;
	}

	// copy over indices
	for (int i = 0; i < obj.getNumIndices(); i++)
	{
		indices[i] = obj.getIndices()[i];
	}

	float3 minVal = float3(9999999e10, 9999999e10, 9999999e10), maxVal = float3(-9999999e10, -9999999e10, -9999999e10);

	// find the minimum and maximum values for the verts
	for (int i = 0; i < obj.getNumVertices(); i++)
	{
		minVal = float3(min(verts[i].position.x, minVal.x), min(verts[i].position.y, minVal.y), min(verts[i].position.z, minVal.z));
		maxVal = float3(max(verts[i].position.x, maxVal.x), max(verts[i].position.y, maxVal.y), max(verts[i].position.z, maxVal.z));
	}

	// compute morton codes
	codes = new uint[obj.getNumIndices() / 3];
	
	for (int i = 0; i < obj.getNumIndices() / 3; i++)
	{
		float xVal = verts[indices[i * 3]].position.x + verts[indices[i * 3 + 1]].position.x + verts[indices[i * 3 + 2]].position.x;
		float yVal = verts[indices[i * 3]].position.y + verts[indices[i * 3 + 1]].position.y + verts[indices[i * 3 + 2]].position.y;
		float zVal = verts[indices[i * 3]].position.z + verts[indices[i * 3 + 1]].position.z + verts[indices[i * 3 + 2]].position.z;
		
		codes[i] = calcMorton((xVal / 3.f - minVal.x) / (maxVal.x - minVal.x),
			(yVal / 3.f - minVal.y) / (maxVal.y - minVal.y),
			(zVal / 3.f - minVal.z) / (maxVal.z - minVal.z));
	}

	// sort the morton codes (put bitmask before sorting)
	uint bitmask = 0;

	for (int i = 18; i < 30; i++)
		bitmask |= 1 << i;

	for (int i = 0; i < obj.getNumIndices() / 3; i++)
		codes[i] &= bitmask;

	sort(codes, codes + obj.getNumIndices() / 3);

	uint sum = 0;

	for (int i = 0; i < obj.getNumIndices() / 3; i++)
		sum += (codes[i] & 1 << 18) != 0 ? 1 : 0;
	printf("%u\n", sum);
	system("PAUSE");
}