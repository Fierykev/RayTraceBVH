#include <RadixSortGlobal.hlsl>
#include <ErrorGlobal.hlsl>

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	if (threadID.x == 0)
	{
		uint offset = 0;

		numOnesBuffer[0] = RS_NO_ERROR;

		uint greaterCheck = 0;
		
		for (uint i = 1; i < DATA_SIZE * numGrps; i++)
		{
			greaterCheck |= BVHTree[i - 1].code < BVHTree[i].code;
			
			if (BVHTree[i - 1 + offset].code > BVHTree[i + offset].code)
			{
				numOnesBuffer[0] = RS_ERROR_1;

				break;
			}
		}

		if (!greaterCheck)
			numOnesBuffer[0] = RS_ERROR_2;
	}
}