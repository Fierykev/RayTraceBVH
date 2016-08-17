#include "RayTraceGlobal.hlsl"

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	for (uint i = 0; i < 5000; i++)
		outputTex[i] = float4(1, 0, 0, 1);
}