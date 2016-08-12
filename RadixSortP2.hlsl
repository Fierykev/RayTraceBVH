#include <RadixSortGlobal.hlsl>

[numthreads(NUM_THREADS, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	// get radixi
	getRadixi(groupThreadID, groupID);

	if (groupThreadID.x == 0)
	{
		// init to zero
		netOnes = 0;

		// sum up the preceding ones
		[loop]
		for (uint sumi = 0; sumi < numGrps; sumi++)
		{
			if (sumi == groupID.x)
				numPrecOnes = netOnes;

			netOnes += numOnesBuffer[sumi];
		}
	}

	// pause for group
	GroupMemoryBarrierWithGroupSync();

	// calculate position if the bit is not present
	[unroll(2)]
	for (uint loadi = 0; loadi < 2; loadi++)
	{
		uint index = (groupThreadID.x << 1) + loadi;
		uint globalIndex = (threadID.x << 1) + loadi;

		uint tmpData;

		if (radixi == 0)
			tmpData = codes[globalIndex];
		else if (radixi & 0x1)
			tmpData = codes[sortedIndexBackBuffer[globalIndex]];
		else
			tmpData = codes[sortedIndex[globalIndex]];

		uint tmpPositionNotPresent =
			transferBuffer[globalIndex];

		uint positionPresent =
			globalIndex
			- tmpPositionNotPresent - numPrecOnes
			+ netOnes;

		uint sIndex =
			((tmpData & (1 << radixi)) ?
				positionPresent :
				tmpPositionNotPresent + numPrecOnes);

		uint lookupIndex = groupID.x * DATA_SIZE + index;

		if (radixi == 0) // startup
			sortedIndexBackBuffer[sIndex] = lookupIndex;
		else if (radixi & 0x1) // odd
			sortedIndex[sIndex] = sortedIndexBackBuffer[lookupIndex];
		else // even
			sortedIndexBackBuffer[sIndex] = sortedIndex[lookupIndex];
	}

	// update the radixi
	incRadixi(groupThreadID, groupID);
}