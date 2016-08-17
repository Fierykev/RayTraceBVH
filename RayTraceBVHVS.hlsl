#include <RayTraceGlobal.hlsl>
#include <ErrorGlobal.hlsl>

struct VSINPUT
{
	float4 position : POSITION;
};

struct PSINPUT
{
	float4 position : SV_POSITION;
};

PSINPUT main(VSINPUT vertIn)
{
	PSINPUT pixelIn;
	pixelIn.position = vertIn.position;

	return pixelIn;
}