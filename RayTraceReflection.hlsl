#include <RayTraceGlobal.hlsl>
#include <RayTraceTraversal.hlsl>

#define RAY_OFFSET .0001

[numthreads(15, 15, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// get global index ID
	uint gloablIndexID = threadID.y * screenWidth + threadID.x;

	// get the reflection rayTmp
	RayPresent rayTmp;
	ColTri colTri;

	// check if this pixel is within bounds
	if (threadID.x < screenWidth && threadID.y < screenHeight
		&& INTENSITY_MIN < (rayTmp = reflectRay[gloablIndexID]).intensity)
	{
		// find the collision with a triangle
		findCollision(rayTmp.ray, colTri);

		if (colTri.collision)
		{
			// get collision location
			const float3 hitLoc = getHitLoc(rayTmp.ray, colTri);

			Material triMat = mat[matIndices[colTri.index]];

			// get the current intensity
			float2 texCoord;
			float3 normal;

			getNromalTexCoord(colTri.tri, hitLoc, texCoord, normal);

			rayTmp.color = lerp(
				rayTmp.color,
				renderPixel(hitLoc,
					triMat, colTri.tri, texCoord
					) * triMat.specular, rayTmp.intensity);

			rayTmp.intensity *= triMat.shininess / 1000.f * REFLECTION_DECAY;

			rayTmp.ray.origin = hitLoc + normal * RAY_OFFSET;
			rayTmp.ray.direction = normalize(reflect(rayTmp.ray.direction,
				normal));
			rayTmp.ray.invDirection = 1.f / rayTmp.ray.direction;
		}
		else // remove the rayTmp
		{
			rayTmp.color = lerp(
				rayTmp.color,
				getBackground(), rayTmp.intensity);


			rayTmp.intensity = 0;
		}

		// store the data
		reflectRay[gloablIndexID] = rayTmp;
	}
}