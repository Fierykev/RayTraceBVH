#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 128
#define NUM_GROUPS 10
#define DATA_SIZE (NUM_THREADS * 2)

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

uint data[DATA_SIZE * NUM_GROUPS];

uint tmpData[DATA_SIZE];

void randomFill()
{
	for (uint i = 0; i < DATA_SIZE; i++)
		data[i] = rand();

	// clone the data
	for (uint i = 0; i < DATA_SIZE; i++)
		tmpData[i] = data[i];

	// scoot to the right
	for (uint i = DATA_SIZE - 1; 0 < i; i--)
		tmpData[i] = tmpData[i - 1];

	tmpData[0] = 0;

	// run the sum
	for (uint i = 0; i < DATA_SIZE - 1; i++)
		tmpData[i + 1] += tmpData[i];
}

void randomFillLocal()
{
	for (uint i = 0; i < DATA_SIZE; i++)
		data[i] = rand();

	// clone the data
	for (uint i = 0; i < DATA_SIZE; i++)
		tmpData[i] = data[i];

	// scoot to the right
	for (uint i = NUM_GROUPS * DATA_SIZE - 1; 0 < i; i--)
		tmpData[i] = tmpData[i - 1];

	tmpData[0] = 0;

	// run the sum
	for (uint i = 0; i < NUM_GROUPS * DATA_SIZE - 1; i++)
		tmpData[i + 1] += tmpData[i];

	// scoot to the right
	for (uint i = DATA_SIZE - 1; 0 < i; i--)
		data[i] = data[i - 1];

	for (uint j = 0; j < NUM_GROUPS; j++)
	{
		data[j * DATA_SIZE] = 0;

		// run the sum
		for (uint i = 0; i < DATA_SIZE - 1; i++)
			data[j * DATA_SIZE + i + 1] += data[j * DATA_SIZE + i];
	}
}

void upSweep()
{
	for (uint upLoop = 1; upLoop < DATA_SIZE / 2; upLoop *= 2)
	{
		for (uint thread = 0; thread < NUM_THREADS; thread++)
		{
			uint index = upLoop - 1 + upLoop * 2 * thread;

			if (index < DATA_SIZE)
				data[index + upLoop] += data[index];
		}
	}
}

void downSweep()
{
	data[DATA_SIZE - 1] = 0;

	for (uint upLoop = DATA_SIZE / 2; upLoop >= 1; upLoop /= 2)
	{
		for (uint thread = 0; thread < NUM_THREADS; thread++)
		{
			uint index = upLoop - 1 + upLoop * 2 * thread;

			if (index < DATA_SIZE)
			{
				uint tmp = data[index];

				data[index] = data[index + upLoop];

				data[index + upLoop] += tmp;
			}
		}
	}
}

void finalSweeps()
{
	uint numOutput = 10;
	

	// run the sumation again but use the sum buffer and allow for larger sizes
	const uint manageNum = ceil(numOutput / (float)NUM_THREADS);
	const uint numLoops = ceil(log(manageNum / 2));

	// up sweep
	for (uint finalUpLoop = 1; finalUpLoop < numLoops; finalUpLoop *= 2)
	{
		for (uint thread = 0; thread < NUM_THREADS; thread++)
		{
			uint index = finalUpLoop - 1 + finalUpLoop * 2 * thread + groupThreadID * DATA_SIZE;

			if (index < DATA_SIZE)
				sumBuffer[index + upLoop] += sumBuffer[index];
		}
	}

}

int main()
{/*
	randomFill();

	for (uint i = 0; i < 10; i++)
		printf("%u ", data[i]);

	printf("\n");

	upSweep();

	downSweep();
	
	for (uint i = 0; i < 10; i++)
		printf("%u ", tmpData[i]);

	printf("\n");

	for (uint i = 0; i < 10; i++)
		printf("%u ", data[i]);
	*/



	system("PAUSE");
	return 0;
}