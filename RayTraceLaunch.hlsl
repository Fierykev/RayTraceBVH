#include <RayTraceGlobal.hlsl>
#include <RayTraceTraversal.hlsl>

#define RAY_OFFSET .001

[numthreads(15, 15, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// get global index ID
	uint gloablIndexID = threadID.y * screenWidth + threadID.x;

	Ray ray;
	RayPresent refRay, refrRay;
	ColTri colTri;

	float halfWidth = screenWidth >> 1;
	float halfHeight = screenHeight >> 1;

	// check if this pixel is within bounds
	if (threadID.x < screenWidth && threadID.y < screenHeight)
	{
		// get the ray position
		ray.origin = float3((threadID.x - halfWidth) / 4.f,
			(threadID.y - halfHeight) / 4.f, 0);

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

			// get the current intensity
			float2 texCoord;
			float3 normal;

			getNromalTexCoord(colTri.tri, hitLoc, texCoord, normal);

			// get the color (add to ref ray for storage)
			refRay.color = renderPixel(hitLoc,
				triMat, colTri.tri, texCoord
				) * triMat.specular;

			if (refRay.intensity != 0)
			{
				refRay.ray.origin = hitLoc + normal * RAY_OFFSET;
				refRay.ray.direction = normalize(reflect(ray.direction,
					normal));
				refRay.ray.invDirection = 1.f / refRay.ray.direction;
			}
			
			// refraction
			refrRay.intensity = (1.f - triMat.alpha) * REFRACTION_DECAY;
			refrRay.color = float4(1.f, 1.f, 1.f, 1.f);

			if (refrRay.intensity != 0)
			{
				refrRay.ray.origin = hitLoc - normal * RAY_OFFSET;
				refrRay.ray.direction = 
					normalize(refract(ray.direction,
					normal, triMat.opticalDensity));
				refrRay.ray.invDirection = 1.f / refrRay.ray.direction;
			}
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