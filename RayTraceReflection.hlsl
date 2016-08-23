#include <RayTraceGlobal.hlsl>
#include <RayTraceTraversal.hlsl>

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

			rayTmp.color = lerp(
				rayTmp.color,
				renderPixel(hitLoc,
					triMat, colTri.tri
					) * triMat.specular, rayTmp.intensity);
			// get the current intensity
			rayTmp.intensity *= triMat.shininess / 1000.f * REFLECTION_DECAY;

			rayTmp.ray.origin = hitLoc;
			rayTmp.ray.direction = normalize(reflect(rayTmp.ray.direction,
				vertexNormaltoFaceNormal(colTri.tri)));
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