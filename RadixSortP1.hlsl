#include <RadixSortGlobal.hlsl>

/**************************************
PREFIX SUM
**************************************/

void prefixSum(uint ID)
{
	// up sweep
	[unroll(8)]
	for (uint upSweepi = 1; upSweepi < DATA_SIZE; upSweepi <<= 1)
	{
		uint indexL = ID * (upSweepi << 1) + upSweepi - 1;
		uint indexR = ID * (upSweepi << 1) + (upSweepi << 1) - 1;

		if (indexR < DATA_SIZE)
			positionNotPresent[indexR] += positionNotPresent[indexL];

		// pause for group
		GroupMemoryBarrierWithGroupSync();
	}

	// down sweep

	if (ID == 0)
		positionNotPresent[DATA_SIZE - 1] = 0;

	// pause for group
	GroupMemoryBarrierWithGroupSync();

	[unroll(8)]
	for (uint downSweepi = DATA_SIZE >> 1; 0 < downSweepi; downSweepi >>= 1)
	{
		uint ID_index = ID * (downSweepi << 1);

		uint index_1 = ID_index + downSweepi - 1;
		uint index_2 = ID_index + (downSweepi << 1) - 1;

		if (index_2 < DATA_SIZE)
		{
			uint tmp = positionNotPresent[ID_index + downSweepi - 1];
			positionNotPresent[index_1] = positionNotPresent[index_2];
			positionNotPresent[index_2] = tmp + positionNotPresent[index_2];
		}

		// pause for group
		GroupMemoryBarrierWithGroupSync();
	}
}

[numthreads(NUM_THREADS, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	// get radixi
	getRadixi(groupThreadID, groupID);

	/***********************************************
	Radix Sort P1
	***********************************************/

	// load in the data

	[unroll(2)]
	for (uint loadi = 0; loadi < 2; loadi++)
	{
		uint index = (groupThreadID << 1) + loadi;

		uint globalIndex = (threadID.x << 1) + loadi;

		uint tmpData;

		if (radixi & 0x1)
			tmpData = BVHTree[globalIndex + numObjects].code;
		else
			tmpData = BVHTree[globalIndex].code;

		// store the inverted version of the important bit
		positionNotPresent[index] = !(tmpData & (1 << radixi));
	}

	// pause for the device accesses
	GroupMemoryBarrierWithGroupSync();
	
	// set the one's count
	if (groupThreadID == 0)
		totalOnes = positionNotPresent[DATA_SIZE - 1];

	// run a prefix sum
	prefixSum(groupThreadID);

	// add the number of ones in this group to a global buffer
	if (groupThreadID == 0)
		numOnesBuffer[groupID.x] = (totalOnes += positionNotPresent[DATA_SIZE - 1]);

	// output the data
	[unroll(2)]
	for (loadi = 0; loadi < 2; loadi++)
	{
		uint index = (groupThreadID << 1) + loadi;
		uint globalIndex = (threadID.x << 1) + loadi;

		transferBuffer[globalIndex] = positionNotPresent[index];
	}
}