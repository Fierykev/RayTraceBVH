#include <RayTraceGlobal.hlsl>

/**
WARNING THE BELOW CODE IS HIGHLY DIVERGENT.
MAY WANT TO LOOK FOR A LESS DIVERGENT ALGORITHM.
**/

float3 minUnion(float3 data1, float3 data2)
{
	return float3(min(data1.x, data2.x),
		min(data1.y, data2.y), min(data1.z, data2.z));
}

float3 maxUnion(float3 data1, float3 data2)
{
	return float3(max(data1.x, data2.x),
		max(data1.y, data2.y), max(data1.z, data2.z));
}

[numthreads(DATA_SIZE, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	int nodeID = BVHTree[threadID.x].parent;

	uint value;

	// atomically add to prevent two threads from entering a node
	InterlockedAdd(transferBuffer[nodeID - numObjects], 1, value);

	[loop]
	while (value)
	{
		// compute the union of the bounding boxes
		BVHTree[nodeID].bbMin = minUnion(BVHTree[BVHTree[nodeID].childL].bbMin,
			BVHTree[BVHTree[nodeID].childR].bbMin);

		BVHTree[nodeID].bbMax = maxUnion(BVHTree[BVHTree[nodeID].childL].bbMax,
			BVHTree[BVHTree[nodeID].childR].bbMax);
		
		// get the parent
		nodeID = BVHTree[nodeID].parent;

		// atomically add to prevent two threads from entering a node
		if (nodeID != -1)
			InterlockedAdd(transferBuffer[nodeID - numObjects], 1, value);
		else
			return;
	}
}