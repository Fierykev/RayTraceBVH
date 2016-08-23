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

return reflectRay[index].color;// *refractRay[index].color;

	//return debugVar[0] ? float4(1, 0, 0, 1) :
		//float4(0, 1, 0, 1);
}