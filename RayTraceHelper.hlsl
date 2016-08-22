#ifndef RAY_TRACE_HELPER_H
#define RAY_TRACE_HELPER_H

#include <RayTraceGlobal.hlsl>

#define magnitude(v) sqrt(v.x * v.x + v.y * v.y + v.z * v.z)

/**
Get the vertex normal.
**/

float3 vertexNormaltoFaceNormal(float3 v0, float3 v1, float3 v2, float3 n0, float3 n1, float3 n2)
{
	float3 p0 = v1 - v0;
	float3 p1 = v2 - v0;
	float3 facenormal = cross(p0, p1);

	float3 vertexnormal = (n0 + n1 + n2) / 3.f; // average the normals
	float direction = dot(facenormal, vertexnormal); // calculate the direction

	return direction < .0f ? -facenormal : facenormal;
}

/**
Get the texture coordinate within the triangle.
**/

float2 getTexCoord(Triangle tri, float3 pt)
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
	return tri.verts[0].texcoord * a1
		+ tri.verts[1].texcoord * a2
		+ tri.verts[2].texcoord * a3;
}


#endif