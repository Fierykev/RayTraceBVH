#define ERRROR .01

struct VERTEX
{
	float3 position;
	float3 normal;
	float2 texcoord;
};

StructuredBuffer<VERTEX> verts : register(t0);
StructuredBuffer<unsigned int> indices : register(t1);

RWStructuredBuffer<unsigned int> bvh : register(u0);


unsigned int bitTwiddling(unsigned int var)
{
	const uint byteMasks[] = { 0x09249249, 0x030c30c3, 0x0300f00f, 0x030000ff, 0x000003ff };
	int curVal = 16;

	[unroll(3)]
	for (int i = 4; 0 < i; i--)
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
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint numIndices, stride;
	indices.GetDimensions(numIndices, stride);

	float3 minPos = (verts[indices[0]].position + verts[indices[1]].position + verts[indices[2]].position) / 3.f, maxPos = minPos;

	// find the bounds of the midpoints
	for (int i = 3; i < numIndices; i += 3)
	{
		float3 midpoint = (verts[indices[i]].position + verts[indices[i + 1]].position + verts[indices[i + 2]].position) / 3.f;

		for (int j = 0; j < 3; j++)
		{
			if (minPos[j] > midpoint[j])
				minPos[j] = midpoint[j];

			if (maxPos[j] < midpoint[j])
				maxPos[j] = midpoint[j];
		}
	}
	
	// start computing mortone codes
	for (uint i = 0; i < numIndices; i += 3)
	{
		float3 scaledMidpoint = ((verts[indices[i]].position + verts[indices[i + 1]].position + verts[indices[i + 2]].position) / 3.f - minPos + ERRROR) / (maxPos - minPos + ERRROR);
		if (calcMortonCode(scaledMidpoint).x < 0.f)
			bvh[0] = 1;
		
	}
	
	/*if (abs(verts[0].position.x) < 8.f && abs(verts[0].position.x) > 7.f && indices[100] == 100)
		bvh[0] = 1;
	else
		bvh[0] = 0;*/
}