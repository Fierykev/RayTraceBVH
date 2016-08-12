#define NUM_THREADS 128
#define DATA_SIZE NUM_THREADS * 2
#define NUM_GRPS 12

cbuffer CONSTANT_BUFFER : register(b0)
{
	uint numGrps;
};

struct VERTEX
{
	float3 position;
	float3 normal;
	float2 texcoord;
};

StructuredBuffer<VERTEX> verts : register(t0);
StructuredBuffer<uint> indices : register(t1);

RWStructuredBuffer<uint> codes : register(u0);
RWStructuredBuffer<uint> sortedIndex : register(u1);
RWStructuredBuffer<uint> sortedIndexBackBuffer : register(u2);
RWStructuredBuffer<uint> numOnesBuffer : register(u3);

groupshared uint indexData[DATA_SIZE * 3];
groupshared uint codeData[DATA_SIZE];

groupshared uint positionNotPresent[DATA_SIZE];

groupshared uint netOnes, numPrecOnes, totalOnes;

groupshared uint tmp[DATA_SIZE];

// TMP
float3 maxMin[] = {
	float3(36,42,44), float3(-51,-2,-43)
};

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

/**************************************
PREFIX SUM
**************************************/

void prefixSum(uint ID)
{
	// up sweep
	//[unroll(8)]
	for (uint upSweepi = 1; upSweepi < DATA_SIZE; upSweepi <<= 1)
	{
		uint indexL = ID * (upSweepi << 1) + upSweepi - 1;
		uint indexR = ID * (upSweepi << 1) + (upSweepi << 1) - 1;

		if (indexR < DATA_SIZE)
			positionNotPresent[indexR] += positionNotPresent[indexL];

		// pause for group
		GroupMemoryBarrierWithGroupSync();
	}
	
	// down sweep

	if (ID == 0)
		positionNotPresent[DATA_SIZE - 1] = 0;

	// pause for group
	GroupMemoryBarrierWithGroupSync();

	//[unroll(8)]
	for (uint downSweepi = DATA_SIZE >> 1; 0 < downSweepi; downSweepi >>= 1)
	{
		uint ID_index = ID * (downSweepi << 1);

		uint index_1 = ID_index + downSweepi - 1;
		uint index_2 = ID_index + (downSweepi << 1) - 1;

		if (index_2 < DATA_SIZE)
		{
			uint tmp = positionNotPresent[ID_index + downSweepi - 1];
			positionNotPresent[index_1] = positionNotPresent[index_2];
			positionNotPresent[index_2] = tmp + positionNotPresent[index_2];
		}

		// pause for group
		GroupMemoryBarrierWithGroupSync();
	}
}

uint rand(uint lfsr)
{
	uint bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
	return lfsr = (lfsr >> 1) | (bit << 15);
}

[numthreads(NUM_THREADS, 1, 1)]
void main(uint threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	/***********************************************
	Generate the Morton Codes (load factor of 2)
	***********************************************/
	
	[unroll(2)]
	for (uint loadi = 0; loadi < 2; loadi++)
	{
		// find the sum of the verts in the triangle
		float3 avg = float3(0, 0, 0);

		// load in the data (3 indices and 3 verts)
		//[unroll(3)]
		for (uint sumi = 0; sumi < 3; sumi++)
		{
			indexData[groupThreadID.x * 3 * 2 + loadi * 3 + sumi] = indices[threadID.x * 3 * 2 + loadi * 3 + sumi];
			avg += verts[indexData[groupThreadID.x * 3 * 2 + loadi * 3 + sumi]].position;
		}

		// divide by three to compute the average
		avg /= 3.f;

		// get the min and max
		float3 max = maxMin[0]; // TODO: FIX FOR SPEED (BAD GLOBAL)
		float3 min = maxMin[1];
		
		// place the centroid point in the unit cube and calculate / store the morton code
		// store globally as well
		// TODO: RESTORE MORTON CODE
		
		float pointData = (float)(groupID.x + 1) * (groupThreadID.x * 2 + loadi + 1);
		
		float3 tmpPoint = float3(
			rand(pointData) / rand(rand(pointData)),
			rand(rand(pointData)) / rand(pointData),
			rand(rand(rand(pointData))) / rand(pointData)
		);
		
		codes[groupID.x * DATA_SIZE + groupThreadID.x * 2 + loadi] = (codeData[groupThreadID.x * 2 + loadi] = calcMortonCode(tmpPoint));//(avg - min) / (max - min)));
	}

	/***********************************************
	Radix Sort the Morton Codes
	***********************************************/	
	//uint radixi = 0;
	//[loop]
	/*
	for (uint radixi = 0; radixi < 1; radixi++) // FIX LOOP NUMBER
	{
		// load in the data if not the first loop
		// (the first loop is already loaded)
		if (radixi != 0)
		{
			//[unroll(2)]
			for (uint loadi = 0; loadi < 2; loadi++)
			{
				uint index = groupThreadID.x * 2 + loadi;

				uint lookupIndex = groupID.x * DATA_SIZE + index;

				if (radixi & 0x1)
					codeData[index] = codes[sortedIndexBackBuffer[lookupIndex]];
				else
					codeData[index] = codes[sortedIndex[lookupIndex]];
			}
			DeviceMemoryBarrierWithGroupSync();
			break;
		}

		

		for (uint loadi = 0; loadi < 2; loadi++)
		{
			uint index = groupThreadID.x * 2 + loadi;

			// store the inverted version of the important bit
			positionNotPresent[index] = !(codeData[index] & (1 << radixi));
		}

		// pause for group
		GroupMemoryBarrierWithGroupSync();

		// set the one's count
		if (groupThreadID.x == 0)
			totalOnes = positionNotPresent[DATA_SIZE - 1];
		
		// run a prefix sum
		prefixSum(groupThreadID.x);
		
		// add the number of ones in this group to a global buffer
		if (groupThreadID.x == 0)
			numOnesBuffer[groupID.x] = (totalOnes += positionNotPresent[DATA_SIZE - 1]);
		
		// global memory sync
		DeviceMemoryBarrierWithGroupSync();
		
		if (groupThreadID.x == 0)
		{
			// init to zero
			netOnes = 0;

			// sum up the preceding ones
			for (uint sumi = 0; sumi < numGrps; sumi++)
			{
				if (sumi == groupID.x)
					numPrecOnes = netOnes;

				netOnes += numOnesBuffer[sumi];
			}
		}
		
		// pause for group
		GroupMemoryBarrierWithGroupSync();
		/*
		// calculate position if the bit is not present
		//[unroll(2)] Unroll causes error
		for (loadi = 0; loadi < 2; loadi++)
		{
			uint index = groupThreadID.x * 2 + loadi;

			uint positionPresent =
				index + groupID.x * DATA_SIZE
				- positionNotPresent[index] - numPrecOnes
				+ netOnes;

			uint sIndex =
				((codeData[index] & (1 << radixi)) ?
					positionPresent :
					positionNotPresent[index] + numPrecOnes);

			uint lookupIndex = groupID.x * DATA_SIZE + index;

			tmp[index] = positionNotPresent[index];
			/*
			if (radixi == 0) // startup
				sortedIndexBackBuffer[sIndex] = lookupIndex;
			else if (radixi & 0x1) // odd
				sortedIndex[sIndex] = codeData[index];// sortedIndexBackBuffer[lookupIndex];
			else // even
				sortedIndexBackBuffer[sIndex] = sortedIndex[lookupIndex];
		*/
		//}
		
		// global memory sync
		//DeviceMemoryBarrierWithGroupSync();
	//}
	numOnesBuffer[0] = 1000;
	DeviceMemoryBarrierWithGroupSync();
	if (groupID.x == 20)//groupThreadID.x == 0 && 
	{
		//sortedCodes[23] = 4;
		//numOnesBuffer[0] = 1000;

		//if (sortedIndexBackBuffer[19 * DATA_SIZE + 4] == 4713)//codeData[4] == 766958445)
		//if (tmp[4] == 1)
			numOnesBuffer[0] = 345;
		/*
		for (uint i = 1; i < DATA_SIZE * numGrps; i++)
		{
			if ((sortedIndex[i - 1] & 0x11) > (sortedIndex[i] & 0x11))
			{
				numOnesBuffer[0] = 345;
				break;
			}
		}

		/*
		for (uint i = 1; i < DATA_SIZE * numGrps; i++)
		{
			//if ((codes[sortedIndex[i - 1]] & (1 << radixi)) > (codes[sortedIndex[i]] & (1 << radixi))) //sortedCodes[i - 1] > sortedCodes[i])
			if ((codes[sortedIndex[i - 1]] & 0x11) > (codes[sortedIndex[i]] & 0x11))
			{
				numOnesBuffer[0] = 345;
				break;
			}
		}*/
	}
}