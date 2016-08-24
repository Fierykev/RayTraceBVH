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
		&& INTENSITY_MIN < (rayTmp = refractRay[gloablIndexID]).intensity
		&& rayTmp.ray.direction.x +
		rayTmp.ray.direction.y +
		rayTmp.ray.direction.z != 0)
	{
		// find the collision with a triangle
		findCollision(rayTmp.ray, colTri);

		if (colTri.collision)
		{
			// get collision location
			const float3 hitLoc = getHitLoc(rayTmp.ray, colTri);

			Material triMat = mat[matIndices[colTri.index]];

			rayTmp.color =
				lerp(
				rayTmp.color,
				renderPixel(hitLoc,
					triMat, colTri.tri
				) * triMat.ambient, rayTmp.intensity);

			// get the current intensity
			rayTmp.intensity *= (1.f - triMat.alpha) * REFRACTION_DECAY;

			rayTmp.ray.origin = hitLoc;
			rayTmp.ray.direction =
				normalize(refract(rayTmp.ray.direction,
				vertexNormalAvg(colTri.tri), triMat.opticalDensity));
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
		refractRay[gloablIndexID] = rayTmp;
	}
}