#include <RadixSortGlobal.hlsl>
#include <ErrorGlobal.hlsl>

bool float3Compare(float3 a, float3 b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z);
}

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	if (threadID.x == 0)
	{
		//numOnesBuffer[0] = RS_NO_ERROR;
		/*
		// random bvh check
		//if (BVHTree[numObjects + 56].childR != 5948)
			//numOnesBuffer[0] = BVH_ERROR_1;

		if (BVHTree[numObjects + 500].childL != 6389)
			numOnesBuffer[0] = BVH_ERROR_1;
			
		//if (BVHTree[numObjects + 56].parent != 5936)
			//numOnesBuffer[0] = BVH_ERROR_1;
		
		if (BVHTree[numObjects + 500].parent != 6391)
			numOnesBuffer[0] = BVH_ERROR_1;
		
		if (numObjects + 500 != 6388)
			numOnesBuffer[0] = BVH_ERROR_1;

		if (numObjects != 5888)
			numOnesBuffer[0] = BVH_ERROR_1;*/

		// if (debugData[0].parent == 5889)
			//numOnesBuffer[0] = BVH_ERROR_1;

		
		for (uint i = 0; i < numObjects; i++)
		{
			if (!float3Compare(debugData[i].bbMin, BVHTree[i].bbMin))
				//|| !float3Compare(debugData[i].bbMax, BVHTree[i].bbMax))
				debugVar[0] = BVH_ERROR_1;
		}
		
		/*
		for (uint i = 0; i < numObjects; i++)
		{
			if (debugData[i].code != transferBuffer[i])//BVHTree[i].code)
				numOnesBuffer[0] = BVH_ERROR_1;
		}

		/*
		float3 match = float3(964, 1445, 2);

		int index = numObjects;
		/*
		for (uint i = numObjects - 10; i < numObjects; i++)
		{
			index = i;

			while (BVHTree[index].parent != -1)
			{
				index = BVHTree[index].parent;
			}

			if (index != numObjects)
			{
				numOnesBuffer[0] = BVH_ERROR_1;
				break;
			}
		}
		/*
		// random bounding box check
		if (BVHTree[index].bbMax.x == 964)
			numOnesBuffer[0] = BVH_ERROR_2;

			/* &&
			BVHTree[index].bbMax.y == match.y &&
			BVHTree[index].bbMax.z == match.z*/
			
	}
}