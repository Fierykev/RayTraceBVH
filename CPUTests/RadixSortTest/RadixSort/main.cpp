#include <stdio.h>
#include <iostream>
#include <bitset>

typedef unsigned int uint;

#define DATA_SIZE 256

#define NUM_GRPS 23

uint sortedCodes[DATA_SIZE * NUM_GRPS], backbufferCodes[DATA_SIZE * NUM_GRPS], codeData[NUM_GRPS][DATA_SIZE];

// gen data
uint positionNotPresent[NUM_GRPS][DATA_SIZE], positionPresent[NUM_GRPS][DATA_SIZE];

uint numOnesBuffer[DATA_SIZE];

uint fakData[] = {
	1, 0, 1, 1, 0, 0, 0, 1
};

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

int main()
{
	uint sum = 0;

	for (uint i = 0; i < DATA_SIZE; i++)
	{
		for (uint j = 0; j < NUM_GRPS; j++)
		{
			float pointData = (i + 1) * (j + 1);

			float point[] = {
				rand(pointData) / rand(rand(pointData)),
				rand(rand(pointData)) / rand(pointData),
				rand(rand(rand(pointData))) / rand(pointData)
			};

			codeData[j][i] = calcMortonCode(point); //rand();

			sum += !(codeData[j][i] & (1 << 2));
		}
	}

	for (uint radixi = 0; radixi < 2; radixi++)
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
			numOnesBuffer[j] = (totalOnes[j] += positionNotPresent[j][DATA_SIZE - 1]);

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
		if ((codeData[x0][y0] & 0x11) > (codeData[x1][y1] & 0x11))
			printf("ERR\n");
	}
	uint data = 189412135418;
	data |= data >> 1;
	data |= data >> 2;
	data |= data >> 4;
	data |= data >> 8;
	data |= data >> 16;

	data = ~data;

	std::bitset<32> out(data);
	std::cout << out << std::endl;

	system("PAUSE");

	return 0;
}