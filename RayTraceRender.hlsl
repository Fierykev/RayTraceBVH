#ifndef RAY_TRACE_RENDER_H
#define RAY_TRACE_RENDER_H

#include <RayTraceHelper.hlsl>

float4 renderPixel(float3 pixelPoint, Material mat, Triangle tri)
{
	// find the uv coord
	const float2 uv = getTexCoord(tri, pixelPoint);

	// get the color for that coord
	const float4 texColor =
		compSample.Sample(diffuseTex[mat.texNum], uv);

	return mat.diffuse + texColor;
}

#endif