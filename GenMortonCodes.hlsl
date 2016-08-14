#include <RadixSortGlobal.hlsl>

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

uint calcMortonCode(in float3 p)
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

		code[i] = bitTwiddling(p[i]);
	}

	// combine codes for the output
	return code[2] | code[1] << 1 | code[0] << 2;
}

#ifdef DEBUG
uint rand(uint lfsr)
{
	uint bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
	return lfsr = (lfsr >> 1) | (bit << 15);
}
#endif

[numthreads(NUM_THREADS, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	/***********************************************
	Generate the Morton Codes (load factor of 2)
	***********************************************/

	//[unroll(2)]
	for (uint loadi = 0; loadi < 2; loadi++)
	{
		// bounding box computation	
		float3 vertData = verts[indices[threadID.x * 3 * 2 + loadi * 3]].position;

		float3 bbMin = vertData, bbMax = vertData;

		// find the sum of the verts in the triangle
		float3 avg = vertData;

		// load in the data (3 indices and 3 verts)
		//[unroll(3)]
		for (uint sumi = 1; sumi < 3; sumi++)
		{
			vertData = verts[indices[threadID.x * 3 * 2 + loadi * 3 + sumi]].position;

			avg += vertData;

			bbMin = min(bbMin, avg);
			bbMax = max(bbMax, avg);
		}

		// divide by three to compute the average
		avg /= 3.f;

		// place the centroid point in the unit cube and calculate / store the morton code
		// store globally as well
#ifdef DEBUG

		float pointData = (float)(groupID.x + 1) * (groupThreadID.x * 2 + loadi + 1);

		float3 tmpPoint = float3(
			rand(pointData) / rand(rand(pointData)),
			rand(rand(pointData)) / rand(pointData),
			rand(rand(rand(pointData))) / rand(pointData)
			);

		BVHTree[(threadID.x << 1) + loadi].code = calcMortonCode(tmpPoint);

		// store bounding box info
		BVHTree[(threadID.x << 1) + loadi].bbMin = tmpPoint;
		BVHTree[(threadID.x << 1) + loadi].bbMax = float3(tmpPoint.y, tmpPoint.z, tmpPoint.x);
#else
		BVHTree[(threadID.x << 1) + loadi].code = calcMortonCode((avg - sceneBBMin) / (sceneBBMax - sceneBBMin));

		// store bounding box info
		BVHTree[(threadID.x << 1) + loadi].bbMin = bbMin;
		BVHTree[(threadID.x << 1) + loadi].bbMax = bbMax;
#endif
	}
}