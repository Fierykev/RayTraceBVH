#ifndef TEST_DATA_H
#define TEST_DATA_H

typedef unsigned int uint;

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

struct float3
{
	float x, y, z;
	// TODO: CHECK ORDER
	float3(float px, float py, float pz)
	{
		x = px;
		y = py;
		z = pz;
	}

	float3()
	{

	}
};

struct NODE
{
	int parent;
	int childL, childR;

	uint code;

	// bounding box calc
	float3 bbMin, bbMax;

	// index start value
	uint index;
};

NODE* constructDebugTree();

#endif