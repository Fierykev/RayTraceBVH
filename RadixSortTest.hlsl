#include <RayTraceGlobal.hlsl>
#include <ErrorGlobal.hlsl>

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	if (threadID.x == 0)
	{
		uint offset = 0;

		uint greaterCheck = 0;
		
		for (uint i = 1; i < DATA_SIZE * numGrps; i++)
		{
			greaterCheck |= BVHTree[i - 1].code < BVHTree[i].code;
			
			if (BVHTree[i - 1 + offset].code > BVHTree[i + offset].code)
			{
				debugVar[0] = RS_ERROR_1;

				break;
			}
		}

		if (!greaterCheck)
			debugVar[0] = RS_ERROR_2;
	}
}