#ifndef RAY_TRACE_RENDER_H
#define RAY_TRACE_RENDER_H

#include <RayTraceHelper.hlsl>

float4 linearBlend(float4 colorA, float4 colorB)
{
	return (colorA + colorB) / 2.f;
}

float4 getBackground()
{
	return float4(.5f, .5f, .5f, 1.0f);
}

float4 renderPixel(float3 pixelPoint, Material mat, Triangle tri)
{
	// find the uv coord
	const float2 uv = getTexCoord(tri, pixelPoint);

	// get the color for that coord
	float4 texColor = float4(1, 1, 1, 1);

	// get the texture if present
	if (mat.texNum != -1)
	{
		texColor = diffuseTex[NonUniformResourceIndex(mat.texNum)].SampleLevel(
			compSample, uv, 0);
	}

	return saturate(mat.ambient + mat.diffuse * texColor);
}

/**
Clear out a ray.
**/

void clearRayPresent(out RayPresent rayPresent, float4 color)
{
	rayPresent.intensity = 0.f;
	rayPresent.ray.direction = 0;
	rayPresent.ray.origin = 0;
	rayPresent.ray.invDirection = 0;
	rayPresent.color = color;
}

#endif