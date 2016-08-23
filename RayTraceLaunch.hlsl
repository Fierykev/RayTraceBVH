#include <RayTraceGlobal.hlsl>
#include <RayTraceTraversal.hlsl>

[numthreads(15, 15, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// get global index ID
	uint gloablIndexID = threadID.y * screenWidth + threadID.x;

	Ray ray;
	RayPresent refRay, refrRay;
	ColTri colTri;

	uint halfWidth = screenWidth >> 1;
	uint halfHeight = screenHeight >> 1;

	// check if this pixel is within bounds
	if (threadID.x < screenWidth && threadID.y < screenHeight)
	{
		// get the ray position
		ray.origin = float3(((float)threadID.x - halfWidth) / 4.f,
			((float)threadID.y - halfHeight) / 4.f, 0);

		// get the ray direction
		ray.direction = float3(0, 0, 1);

		// get the inverse of the direction
		ray.invDirection = 1.f / ray.direction;

		// find the collision with a triangle
		findCollision(ray, colTri);

		if (colTri.collision)
		{
			// get collision location
			const float3 hitLoc = getHitLoc(ray, colTri);

			Material triMat = mat[matIndices[colTri.index]];

			// TODO: add refraction and shadow

			// store the reflected ray if needed
			// TODO: check specular color

			// reflection
			refRay.intensity = triMat.shininess / 1000.f * REFLECTION_DECAY;

			// get the color (add to ref ray for storage)
			refRay.color = renderPixel(hitLoc,
				triMat, colTri.tri
				) * triMat.specular;

			if (refRay.intensity != 0)
			{
				refRay.ray.origin = hitLoc;
				refRay.ray.direction = normalize(reflect(ray.direction,
					vertexNormaltoFaceNormal(colTri.tri)));
				refRay.ray.invDirection = 1.f / refRay.ray.direction;
			}

			// refraction
			refrRay.intensity = (1.f - triMat.alpha) * REFRACTION_DECAY;

			if (refrRay.intensity != 0)
			{
				refrRay.ray.origin = hitLoc;
				refrRay.ray.direction = normalize(refract(refRay.ray.direction,
					vertexNormaltoFaceNormal(colTri.tri), triMat.opticalDensity));
				refrRay.ray.invDirection = 1.f / refrRay.ray.direction;
			}

			// TODO: add shadow

		}
		else
		{
			// store a miss for all rays
			clearRayPresent(refRay, getBackground());
			clearRayPresent(refrRay, getBackground());
		}

		// store the rays
		reflectRay[gloablIndexID] = refRay;
		refractRay[gloablIndexID] = refrRay;
	}
}