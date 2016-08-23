#ifndef RAY_TRACE_TRAVERSAL_H
#define RAY_TRACE_TRAVERSAL_H

#include <RayTraceGlobal.hlsl>
#include <RayTraceRender.hlsl>

#define EPSILON .0001

#define STACK_SIZE 2<<4

// NOTE: fix performance hit from thread num

/*
Get the hit location from a ray and distance.
*/

float3 getHitLoc(Ray ray, ColTri colTri)
{
	return ray.origin +
		ray.direction * colTri.distance;
}

/*
Update the vert and pass back a new vert.
*/

Vertex getUpdateVerts(uint index)
{
	Vertex updateVert;
	updateVert.position = (float3)mul(float4(verts[index].position, 1),
		worldViewProjection);
	updateVert.normal = mul(verts[index].normal,
		(float3x3)worldView);
	updateVert.texcoord = verts[index].texcoord;

	return updateVert;
}

/*
Find the intersection between a ray and a triangle.
*/

float rayTriangleCollision(Ray ray, Triangle tri)
{
	const float3 edge1 = tri.verts[1].position - tri.verts[0].position;
	const float3 edge2 = tri.verts[2].position - tri.verts[0].position;

	float3 tmpVar = cross(ray.direction, edge2);
	const float dx = dot(edge1, tmpVar);

	// no determinant
	if (abs(dx) < EPSILON)
		return -1.f;

	// calc the inverse determinant
	const float idx = 1.f / dx;

	const float3 rayToTri = ray.origin - tri.verts[0].position;

	// compute the inverse of the 3x3 matrix

	float3 inverse;

	// compute inverse x
	inverse.x = dot(rayToTri, tmpVar) * idx;

	if (inverse.x < .0f || 1.f < inverse.x)
		return -1.f;

	// update with cross product
	tmpVar = cross(rayToTri, edge1);

	// compute inverse y
	inverse.y = dot(ray.direction, tmpVar) * idx;

	// intersection is not within the triangle bounds
	if (inverse.y < .0f || 1.f < inverse.x + inverse.y)
		return -1.f;

	// calculate inverse z (distance)
	inverse.z = dot(edge2, tmpVar) * idx;

	// ray intersects the triangle
	if (EPSILON < inverse.z)
		return inverse.z;

	return -1.f;
}

/*
The slab method is used for collision.
*/

bool rayBoxCollision(Ray ray, Box box, bool collision, float distance)
{
	const float3 deltaMin = (box.bbMin - ray.origin) * ray.invDirection;
	const float3 deltaMax = (box.bbMax - ray.origin) * ray.invDirection;

	const float3 minVector = min(deltaMin, deltaMax);
	const float3 maxVector = max(deltaMin, deltaMax);

	const float minVal = max(max(minVector.x, minVector.y), minVector.z);
	const float maxVal = min(min(maxVector.x, maxVector.y), maxVector.z);

	return 0 <= maxVal && minVal <= maxVal && (!collision || minVal <= distance);
}

void findCollision(Ray ray, out ColTri colTri)
{
	// set no collision
	colTri.collision = false;
	colTri.distance = 0;

	// create the stack vars (1 item is unusable in the stack
	// for controle flow reasons)
	int stackIndex = 0;
	int stack[STACK_SIZE];

	stack[0] = -1;

	// start from the root
	int nodeID = numObjects;

	// tmp vars
	int leftChildNode, rightChildNode;

	bool leftChildCollision, rightChildCollision;
	int leftLeaf, rightLeaf;

	Triangle triTmp;
	float distance;

	uint index;

	do
	{
		// record the index of the children
		leftChildNode = BVHTree[nodeID].childL;
		rightChildNode = BVHTree[nodeID].childR;

		// check for leaf node
		if (leftChildNode == -1 && rightChildNode == -1)
		{
			// get the index
			index = BVHTree[nodeID].index;

			// get triangle
			[unroll(3)]
			for (uint i = 0; i < 3; i++)
				triTmp.verts[i] = getUpdateVerts(indices[index + i]);

			// run collission test
			distance = rayTriangleCollision(ray, triTmp);

			//  update the triangle
			if (distance != -1 && (!colTri.collision || distance < colTri.distance))
			{
				// set index for material reasons later
				colTri.index = index / 3;

				// set collision to true
				colTri.collision = true;

				// store the vertex
				colTri.tri = triTmp;
				colTri.distance = distance;
			}

			// remove the stack top
			nodeID = stack[stackIndex--];

			continue;
		}

		// check for collisions with children
		leftChildCollision = rayBoxCollision(ray, BVHTree[leftChildNode].bbox, colTri.collision, colTri.distance);
		rightChildCollision = rayBoxCollision(ray, BVHTree[rightChildNode].bbox, colTri.collision, colTri.distance);

		if (!leftChildCollision && !rightChildCollision)
		{
			// remove the stack top
			nodeID = stack[stackIndex--];
		}
		else
		{
			// store the right node on the stack if both right and left
			// boxes intersect the ray
			if (leftChildCollision && rightChildCollision)
				stack[++stackIndex] = rightChildNode;

			// update the nodeID depending on if the left side intersected
			nodeID = leftChildCollision ? leftChildNode : rightChildNode;
		}
	} while (stackIndex != -1);
}

#endif