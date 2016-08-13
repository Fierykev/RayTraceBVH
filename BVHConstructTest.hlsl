#include <RadixSortGlobal.hlsl>
#include <ErrorGlobal.hlsl>

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	if (threadID.x == 0)
	{
		numOnesBuffer[0] = RS_NO_ERROR;

		if (BVHTree[8].childL != 11)
			numOnesBuffer[0] = BVH_ERROR_1;
	}
}