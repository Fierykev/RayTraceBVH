#define NUM_THREADS 128
#define DATA_SIZE NUM_THREADS * 2
#define NUM_GRPS 12

RWStructuredBuffer<uint> sumBuffer : register(u0); // only used to store sums (index zero has the min value in it)
RWStructuredBuffer<uint> codes : register(u1);
RWStructuredBuffer<uint> sortedCodes : register(u2);

groupshared uint data[DATA_SIZE];

[numthreads(NUM_THREADS, 1, 1)]
void main(uint threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	uint numCodes, stride;
	codes.GetDimensions(numCodes, stride);

	const uint numOutput = ceil(numCodes / DATA_SIZE);
	uint radixLoop = 0;// goes up to 12

	//for (uint )
	{
		// get a portion of the global memory and add it to shared (load two at a time)
		//[unroll(2)]
		for (uint loadi = 0; loadi < 2; loadi++)
		{
			uint index = threadID * 2 + loadi;

			data[groupThreadID * 2 + loadi] = index < numCodes ? ((codes[index] & 1 << (radixLoop + 18)) != 0 ? 0 : 1) : 0;
		}

		// wait for the other threads in this group to finish
		GroupMemoryBarrierWithGroupSync();

		// up sweep
		for (uint upLoop = 1; upLoop < NUM_THREADS; upLoop *= 2)
		{
			uint index = upLoop - 1 + upLoop * 2 * groupThreadID;

			if (index < DATA_SIZE)
				data[index + upLoop] += data[index];

			// wait for the other threads in this group to finish
			GroupMemoryBarrierWithGroupSync();
		}

		// store the sum of the block into a buffer
		if (groupThreadID == 0)
			sumBuffer[groupID.x] = data[DATA_SIZE - 1];

		// down sweep
		for (uint upLoop = NUM_THREADS; upLoop >= 1; upLoop /= 2)
		{
			uint index = upLoop - 1 + upLoop * 2 * groupThreadID;

			if (index < DATA_SIZE)
			{
				uint tmp = data[index];

				data[index] = data[index + upLoop];

				data[index + upLoop] += tmp;
			}

			// wait for the other threads in this group to finish
			GroupMemoryBarrierWithGroupSync();
		}

		// run the sumation again but use the sum buffer and allow for larger sizes
		const uint manageNum = ceil(numOutput / (float)NUM_THREADS);
		const uint numLoops = ceil(log(manageNum / 2));

		// up sweep
		for (uint finalUpLoop = 1; finalUpLoop < numLoops; finalUpLoop *= 2)
		{
			uint index = upLoop - 1 + upLoop * 2 * groupThreadID + groupThreadID * DATA_SIZE;

			if (index < DATA_SIZE)
				sumBuffer[index + upLoop] += sumBuffer[index];

			// wait for the other threads in this group to finish
			GroupMemoryBarrierWithGroupSync();
		}

		// down sweep
		for (uint upLoop = numLoops; upLoop >= 1; upLoop /= 2)
		{
			uint index = upLoop - 1 + upLoop * 2 * groupThreadID + groupThreadID * DATA_SIZE;

			if (index < DATA_SIZE)
			{
				uint tmp = data[index];

				sumBuffer[index] = sumBuffer[index + upLoop];

				sumBuffer[index + upLoop] += tmp;
			}

			// wait for the other threads in this group to finish
			GroupMemoryBarrierWithGroupSync();
		}
	}
}