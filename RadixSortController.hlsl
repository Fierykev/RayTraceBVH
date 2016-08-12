#include <RadixSortGlobal.hlsl>
#include <RadixSortMortonCode.hlsl>
#include <RadixSortP1.hlsl>
#include <RadixSortP2.hlsl>

[numthreads(NUM_THREADS, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	if (groupThreadID.x == 0)
	{
		radixi = radixiBuffer[groupID.x];
		phase = restart[0] ? 0 : phaseBuffer[groupID.x];

#ifdef DEBUG
		if (restart[0] == 2)
			phase = 3;
#endif
	}

	GroupMemoryBarrierWithGroupSync();
	
	switch (phase)
	{
		case 0:
			// morton codes
			radixSortMortonCode(threadID, groupThreadID, groupID);
			
			// reset radix buffer
			if (threadID.x == 0)
				radixi = 0;
			break;
		case 1:
			// prefix sum
			radixSortP1(threadID, groupThreadID, groupID);
			break;
		case 2:
			// radix sort
			radixSortP2(threadID, groupThreadID, groupID);

			break;
#ifdef DEBUG
		default:
			if (threadID.x == 0)
			{
				numOnesBuffer[0] = 1000;
				
				for (uint i = 1; i < DATA_SIZE * numGrps; i++)
				{
					if ((codes[sortedIndexBackBuffer[i - 1]]) > (codes[sortedIndexBackBuffer[i]]))
					{
						numOnesBuffer[0] = 345;

						break;
					}
				}
			}
			break;
#endif
	}

	// update phase and radixi
	AllMemoryBarrierWithGroupSync();

	if (groupThreadID.x == 0)
	{
		// only accept phase >= 2 for increase
		radixiBuffer[groupID.x] = radixi + (phase >> 1);

		phaseBuffer[groupID.x] = phase % 2 + 1;
	}
}