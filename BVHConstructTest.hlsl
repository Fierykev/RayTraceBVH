#include <RayTraceGlobal.hlsl>
#include <ErrorGlobal.hlsl>

bool float3Compare(float3 a, float3 b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z);
}

float float3Diff(float3 a, float3 b)
{
	return abs(a.x - b.x + a.y - b.y + a.z - b.z) < .0001f;
}

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	if (threadID.x == 0)
	{
		
		for (uint i = 0; i < numObjects; i++)
		{//debugData[i].index
			//if (abs(debugData[i].bbox.bbMin.x - BVHTree[i].bbox.bbMin.x) > .0001f)
			if (debugData[i].bbox.bbMax.z != BVHTree[i].bbox.bbMax.z)
				debugVar[0] = BVH_ERROR_1;
		}
	}
}