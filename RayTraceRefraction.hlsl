#include <RayTraceGlobal.hlsl>
#include <RayTraceTraversal.hlsl>

[numthreads(15, 15, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// get global index ID
	uint gloablIndexID = threadID.y * screenWidth + threadID.x;

	// get the reflection ray
	RayPresent refRay;
	ColTri colTri;

	// check if this pixel is within bounds
	if (threadID.x < screenWidth && threadID.y < screenHeight
		&& INTENSITY_MIN < (refRay = reflectRay[gloablIndexID]).intensity)
	{
		// find the collision with a triangle
		findCollision(refRay.ray, colTri);

		if (colTri.collision)
		{
			// get collision location
			const float3 hitLoc = getHitLoc(refRay.ray, colTri);

			Material triMat = mat[matIndices[colTri.index / 3]];

			refRay.color *= renderPixel(hitLoc,
				triMat, colTri.tri
				) * refRay.intensity;

			// get the current intensity
			refRay.intensity *= (1.f - triMat.alpha) * REFRACTION_DECAY;

			refRay.ray.origin = hitLoc;
			refRay.ray.direction = normalize(refract(refRay.ray.direction,
				vertexNormaltoFaceNormal(colTri.tri), triMat.opticalDensity));
			refRay.ray.invDirection = 1.f / refRay.ray.direction;
		}
		else // remove the ray
		{
			refRay.color *= refRay.intensity;

			refRay.intensity = 0;
		}

		// store the data
		refractRay[gloablIndexID] = refRay;
	}
}