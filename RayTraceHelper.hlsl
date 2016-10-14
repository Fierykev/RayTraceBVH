#ifndef RAY_TRACE_HELPER_H
#define RAY_TRACE_HELPER_H

#include <RayTraceGlobal.hlsl>

#define magnitude(v) sqrt(v.x * v.x + v.y * v.y + v.z * v.z)

/**
Get the texture coordinate within the triangle.
**/

void getNromalTexCoord(Triangle tri, float3 pt, out float2 texcoord, out float3 normal)
{
	// get the vector from the point to the triangle verts
	float3 vector0 = tri.verts[0].position - pt;
	float3 vector1 = tri.verts[1].position - pt;
	float3 vector2 = tri.verts[2].position - pt;

	// calculate areas as well as factors
	float a0 = magnitude(cross(tri.verts[0].position - tri.verts[1].position,
		tri.verts[0].position - tri.verts[2].position)); // main triangle area
	float a1 = magnitude(cross(vector1, vector2)) / a0; // v0 area / a0
	float a2 = magnitude(cross(vector2, vector0)) / a0; // v1 area / a0
	float a3 = magnitude(cross(vector0, vector1)) / a0; // v2 area / a0

	// find the uv coord
	texcoord = tri.verts[0].texcoord * a1
		+ tri.verts[1].texcoord * a2
		+ tri.verts[2].texcoord * a3;

	// find the normal
	normal = tri.verts[0].normal * a1
		+ tri.verts[1].normal * a2
		+ tri.verts[2].normal * a3;
}


#endif