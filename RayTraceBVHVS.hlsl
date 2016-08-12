#include <RadixSortGlobal.hlsl>
#include <ErrorGlobal.hlsl>

/*cbuffer WorldBuffer : register(b0)
{
	matrix worldViewProjection : packoffset(c0);
};
*/
struct VSINPUT
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD0;
};

struct PSINPUT
{
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD0;
	float4 position : SV_POSITION;
	float4 worldpos : TEXCOORD1;
};

PSINPUT main(VSINPUT In)
{
	PSINPUT O;
	matrix worldViewProjection2;
	worldViewProjection2[0] = float4(1, 0, 0, 0);
	worldViewProjection2[1] = float4(0, .995037, -.0995037, 0);
	worldViewProjection2[2] = float4(0, .0995037, .995037, -100.499);
	worldViewProjection2[3] = float4(0, 0, 0, 1);
	
	if (numOnesBuffer[0] != RS_NO_ERROR)
		In.position = float4(0, 0, 0, 0);
	O.position = mul(In.position, worldViewProjection2);

	return O;
}