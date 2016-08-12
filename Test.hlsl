// TODO: find the actual max and min
#define MAX_FLOAT 9999999e-10
#define MIN_FLOAT -9999999e-10

#define NUM_THREADS 128
#define DATA_SIZE NUM_THREADS * 2

struct VERTEX
{
	float3 position;
	float3 normal;
	float2 texcoord;
};

//StructuredBuffer<VERTEX> verts : register(t0);
//StructuredBuffer<unsigned int> indices : register(t1);

//RWStructuredBuffer<float3> outputMax : register(u0);
//RWStructuredBuffer<float3> outputMin : register(u1);

groupshared float3 reductionDataMax[DATA_SIZE];
groupshared float3 reductionDataMin[DATA_SIZE];

/**************************************
MIN / MAX CALC
**************************************/

float3 findMaxVal(float3 p0, float3 p1)
{
	return float3(max(p0.x, p1.x), max(p0.y, p1.y), max(p0.z, p1.z));
}

float3 findMinVal(float3 p0, float3 p1)
{
	return float3(min(p0.x, p1.x), min(p0.y, p1.y), min(p0.z, p1.z));
}

[numthreads(NUM_THREADS, 1, 1)]
void main(uint threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	//uint numVerts, stride;
	//verts.GetDimensions(numVerts, stride);

	//const uint numOutput = ceil(numVerts / DATA_SIZE);
	//if (indices[5] == 4)
	//outputMax[0].x = 3.f;
	/*
	// get a portion of the global memory and add it to shared (load two at a time)
	[unroll(2)]
	for (uint loadi = 0; loadi < 2; loadi++)
	{
	uint index = threadID * 2 + loadi;

	reductionDataMax[groupThreadID * 2 + loadi] = index < numVerts ? verts[index].position : float3(MIN_FLOAT, MIN_FLOAT, MIN_FLOAT);
	reductionDataMin[groupThreadID * 2 + loadi] = index < numVerts ? verts[index].position : float3(MAX_FLOAT, MAX_FLOAT, MAX_FLOAT);
	}

	GroupMemoryBarrierWithGroupSync();

	// start the reduction
	for (uint iRed = 1; iRed < DATA_SIZE; iRed *= 2)
	{
	uint index = iRed * 2 * groupThreadID;

	if (index < DATA_SIZE)
	{
	reductionDataMax[index] = findMaxVal(reductionDataMax[index], reductionDataMax[index + iRed]);
	reductionDataMin[index] = findMinVal(reductionDataMin[index], reductionDataMin[index + iRed]);
	}

	// wait for the other threads in this group to finish
	GroupMemoryBarrierWithGroupSync();
	}

	// write the data to global
	if (groupThreadID == 0)
	{
	outputMax[groupID.x].x = reductionDataMax[0].x;
	outputMin[groupID.x].x = reductionDataMin[0].x;
	}

	// wait for the other thread groups to finish to preform the final reduction
	DeviceMemoryBarrierWithGroupSync();

	// finish the reduction by looking at all group outputs
	uint remVerts = numOutput / DATA_SIZE;
	uint power = 1;
	uint groupOutputNum = ceil(log(numOutput) / log(DATA_SIZE));

	for (uint fLoop = 0; fLoop < groupOutputNum; fLoop++)
	{
	// no more need for this group
	if (remVerts < groupID.x)
	continue; // TODO: change to return when putting in shader

	// fill up the groupshared buffer
	for (uint loadf = 0; loadf < 2; loadf++)
	{
	uint index = (groupID.x * DATA_SIZE + groupThreadID * 2 + loadf) * power;

	reductionDataMax[groupThreadID * 2 + loadf] = index < numOutput ? outputMax[index] : float3(MIN_FLOAT, MIN_FLOAT, MIN_FLOAT);
	reductionDataMin[groupThreadID * 2 + loadf] = index < numOutput ? outputMin[index] : float3(MAX_FLOAT, MAX_FLOAT, MAX_FLOAT);
	}

	// wait for the other threads in this group to finish
	GroupMemoryBarrierWithGroupSync();

	// continue the reduction
	for (uint fRed = 1; fRed < DATA_SIZE; fRed *= 2)
	{
	uint index = fRed * 2 * groupThreadID;

	if (index < DATA_SIZE)
	{
	reductionDataMax[index] = findMaxVal(reductionDataMax[index], reductionDataMax[index + fRed]);
	reductionDataMin[index] = findMinVal(reductionDataMin[index], reductionDataMin[index + fRed]);
	}
	}

	// write the data to global
	if (groupThreadID.x == 0)
	{
	outputMax[groupID.x * DATA_SIZE * power] = reductionDataMax[0];
	outputMin[groupID.x * DATA_SIZE * power] = reductionDataMin[0];
	}

	// add in groupID.x == 0
	remVerts /= DATA_SIZE;

	// multiply the power
	power *= DATA_SIZE;

	// wait for the other thread groups to finish to preform the final reduction
	DeviceMemoryBarrierWithGroupSync();
	}*/
}