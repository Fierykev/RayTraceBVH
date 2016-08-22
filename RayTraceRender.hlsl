#ifndef RAY_TRACE_RENDER_H
#define RAY_TRACE_RENDER_H

#include <RayTraceHelper.hlsl>

float4 renderPixel(float3 pixelPoint, Material mat, Triangle tri)
{
	// find the uv coord
	const float2 uv = getTexCoord(tri, pixelPoint);

	// get the color for that coord
	float4 texColor = float4(0, 0, 0, 1);

	// get the texture if present
	if (mat.texNum != -1)
		texColor =
			diffuseTex[mat.texNum].SampleLevel(
				compSample, uv, 0);

	return saturate(mat.diffuse + texColor);
}

#endif