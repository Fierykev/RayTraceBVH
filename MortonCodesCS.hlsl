struct VERTEX
{
	float3 position;
	float3 normal;
	float2 texcoord;
};

StructuredBuffer<VERTEX> verts : register(t0);
StructuredBuffer<unsigned int> indices : register(t1);
StructuredBuffer<float3> maxMin : register(t2);

RWStructuredBuffer<uint> codes : register(u0);

/**************************************
MORTON CODES
**************************************/

uint bitTwiddling(uint var)
{
	const uint byteMasks[] = { 0x09249249, 0x030c30c3, 0x0300f00f, 0x030000ff, 0x000003ff };
	uint curVal = 16;

	//[unroll(3)]
	for (uint i = 4; 0 < i; i--)
	{
		var &= byteMasks[i]; // set everything other than the last 10 bits to zero
		var |= var << curVal; // add in the byte offset

		curVal /= 2; // divide by two
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

		// check if too low
		if (p[i] < 0.f)
			p[i] = 0.f;

		// check if too high		
		if (p[i] >= 1024.f)
			p[i] = 1023.f;

		code[i] = bitTwiddling(p[i]);
	}

	// combine codes for the output
	return code[2] | code[1] << 1 | code[0] << 2;
}

[numthreads(1, 1, 1)]
void main(uint threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	// find the sum of the verts in the triangle

	float3 avg = float3(0, 0, 0);

	// load in the data (3 indices and 3 verts)
	//[unroll(3)]
	for (uint sumi = 0; sumi < 3; sumi++)
		avg += verts[indices[threadID.x * 3 + sumi]].position;
	
	// divide by three to compute the average
	avg /= 3.f;

	// get the min and max
	float3 max = maxMin[0];
	float3 min = maxMin[1];

	// place the centroid point in the unit cube and calculate / store the morton code
	codes[groupID.x] = calcMortonCode((avg - min) / (max - min));
}