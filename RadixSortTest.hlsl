#include <RadixSortGlobal.hlsl>
#include <ErrorGlobal.hlsl>

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	if (threadID.x == 0)
	{
		numOnesBuffer[0] = 1000;

		uint greaterCheck = 0;
		
		for (uint i = 1; i < DATA_SIZE * numGrps; i++)
		{
			greaterCheck |= codes[sortedIndexBackBuffer[i - 1]] < codes[sortedIndexBackBuffer[i]];
			
			if (codes[sortedIndexBackBuffer[i - 1]] > codes[sortedIndexBackBuffer[i]])
			{
				numOnesBuffer[0] = RS_ERROR_1;

				break;
			}
		}

		if (!greaterCheck)
			numOnesBuffer[0] = RS_ERROR_2;
	}
}