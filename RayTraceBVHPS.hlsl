#include <RayTraceGlobal.hlsl>
#include <ErrorGlobal.hlsl>

struct PSINPUT
{
	float4 position : SV_POSITION;
};

float4 main(PSINPUT In) : SV_TARGET
{
	// get the pixel corresponding the screen position

	uint index = (screenHeight - In.position.y) * screenWidth
		+ (In.position.x - (screenWidth >> 1));

	return reflectRay[index].color;
}