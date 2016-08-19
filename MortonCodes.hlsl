#include <RayTraceGlobal.hlsl>

float3 getPosTrans(uint index)
{
	return (float3)mul(float4(verts[index].position, 1),
		worldViewProjection);
}

/**************************************
MORTON CODES
**************************************/

uint bitTwiddling(in uint var)
{
	const uint byteMasks[] = { 0x09249249, 0x030c30c3, 0x0300f00f, 0x030000ff, 0x000003ff };
	uint curVal = 16;

	[unroll(4)]
	for (uint i = 4; 0 < i; i--)
	{
		var &= byteMasks[i]; // set everything other than the last 10 bits to zero
		var |= var << curVal; // add in the byte offset

		curVal >>= 1; // divide by two
	}

	// add in the final mask
	var &= byteMasks[0];

	return var;
}

uint calcMortonCode(float3 p)
{
	uint3 code;

	// multiply out
	[unroll(3)]
	for (int i = 0; i < 3; i++)
	{
		// change to base .5
		p[i] *= 1024.f;
		
		// clamp between [0, 1024)
		p[i] = clamp(p[i], 0, 1023);

		code[i] = bitTwiddling((uint)p[i]);
	}

	// combine codes for the output
	return code[2] | code[1] << 1 | code[0] << 2;
}

[numthreads(NUM_THREADS, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	/***********************************************
	Generate the Morton Codes (load factor of 2)
	***********************************************/

	float3 bbMin, bbMax;
	float3 avg;

	float3 vertData;

	uint index, indicesBase, mortonCode;

	// note unroll causes error
	//[loop]
	for (uint loadi = 0; loadi < 2; loadi++)
	{
		bbMin = float3(0, 0, 0);
		bbMax = float3(0, 0, 0);
		avg = float3(0, 0, 0);

		index = (threadID.x * 2) + loadi;

		indicesBase = threadID.x * 3 * 2 + loadi * 3;
		
		// check if data is in bounds
		if (indicesBase < numIndices)
		{
			// bounding box computation	
			vertData = getPosTrans(indices[indicesBase]);

			// set the start value
			bbMin = bbMax = avg = vertData;

			// load in the data (3 indices and 3 verts)
			[unroll(2)]
			for (uint sumi = 1; sumi < 3; sumi++)
			{
				vertData = getPosTrans(indices[indicesBase + sumi]);

				bbMin = minUnion(bbMin, vertData);
				bbMax = maxUnion(bbMax, vertData);

				avg = minUnion(bbMin, vertData);
			}

			// divide by three to compute the average
			avg /= 3.f;

			mortonCode = calcMortonCode((avg - sceneBBMin) /
				(sceneBBMax - sceneBBMin));
		}
		
		// place the centroid point in the unit cube and calculate / store the morton code
		// store globally as well

		// store the code
		BVHTree[index].code = mortonCode;

		// store bounding box info
		BVHTree[index].bbox.bbMin = bbMin;
		BVHTree[index].bbox.bbMax = bbMax;

		// set children to invalid
		BVHTree[index].childL = -1;
		BVHTree[index].childR = -1;
		
		// store vert index data
		BVHTree[index].index = indicesBase;
	}
}