#define NUM_THREADS 128

struct VERTEX
{
	float3 position;
	float3 normal;
	float2 texcoord;
};

StructuredBuffer<VERTEX> verts : register(t0);
StructuredBuffer<unsigned int> indices : register(t1);

RWStructuredBuffer<unsigned int> bvh : register(u0);

//groupshared float3 reductionData[NUM_THREADS];

float3 findMaxVal(float3 p0, float3 p1)
{
	return float3(max(p0.x, p1.x), max(p0.y, p0.y), max(p0.z, p0.z));
}

[numthreads(1, 1, 1)]
void main(uint3 groupThreadID : SV_DispatchThreadID)//SV_GroupThreadID)//, uint3 groupID : SV_GroupID)
{
	/*uint numVerts, stride;
	verts.GetDimensions(numVerts, stride);

	uint vertsPerThread = ceil(NUM_THREADS / (float) numVerts);

	// get a portion of the global memory and add it to shared
	for (int i = 0; i < vertsPerThread; i++)
	{
		reductionData[groupThreadID.x * vertsPerThread + i] = groupThreadID.x * vertsPerThread + i < numVerts
			? verts[groupID.x * NUM_THREADS * vertsPerThread + groupThreadID.x + i].position : float3(0, 0, 0);
	}
	
	GroupMemoryBarrierWithGroupSync();
	*/
	if (groupThreadID.x == 10)
		bvh[0] == 1;
		/*
	// start the average
	for (int i = 0; i < vertsPerThread; i *= 2)
	{
		int groupIndex = i * 2 * groupThreadID.x;

		// find new max
		//if (groupIndex < groupID.x)
			//reductionData[groupIndex] = findMaxVal(reductionData[groupIndex], reductionData[groupIndex + i]);
	
		GroupMemoryBarrierWithGroupSync();
	}*/
}