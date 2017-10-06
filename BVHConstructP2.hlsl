#include <RayTraceGlobal.hlsl>

/**
WARNING THE BELOW CODE IS HIGHLY DIVERGENT.
MAY WANT TO LOOK FOR A LESS DIVERGENT ALGORITHM.
**/

[numthreads(DATA_SIZE, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	uint nodeID = BVHTree[threadID.x].parent;

	uint value;

	// atomically add to prevent two threads from entering a node
	InterlockedAdd(transferBuffer[nodeID - numObjects], 1, value);

	[loop]
	while (value)
	{
		// compute the union of the bounding boxes
		BVHTree[nodeID].bbox.bbMin = minUnion(BVHTree[BVHTree[nodeID].childL].bbox.bbMin,
			BVHTree[BVHTree[nodeID].childR].bbox.bbMin);

		BVHTree[nodeID].bbox.bbMax = maxUnion(BVHTree[BVHTree[nodeID].childL].bbox.bbMax,
			BVHTree[BVHTree[nodeID].childR].bbox.bbMax);

		// get the parent
		nodeID = BVHTree[nodeID].parent;

		// atomically add to prevent two threads from entering a node
		if (nodeID != UINT_MAX)
			InterlockedAdd(transferBuffer[nodeID - numObjects], 1, value);
		else
			return;
	}
}
