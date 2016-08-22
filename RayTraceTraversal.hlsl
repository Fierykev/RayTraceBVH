#include <RayTraceGlobal.hlsl>
//#include <RayTraceRender.hlsl>

#define EPSILON .00001

#define STACK_SIZE 2<<5

// NOTE: fix performance hit from thread num

/*
Update the vert and pass back a new vert.
*/

Vertex getUpdateVerts(uint index)
{
	Vertex updateVert;
	updateVert.position = mul(float4(verts[index].position, 1),
		worldViewProjection);
	updateVert.normal = mul(float4(verts[index].normal, 1),
		world);
	updateVert.texcoord = verts[index].texcoord;

	return updateVert;
}

/*
Find the intersection between a ray and a triangle.
*/

float rayTriangleCollision(Ray ray, Triangle tri)
{
	float3 edge1 = tri.verts[1].position - tri.verts[0].position;
	float3 edge2 = tri.verts[2].position - tri.verts[0].position;

	float3 tmpVar = cross(ray.direction, edge2);
	float dx = dot(edge1, tmpVar);

	// no determinant
	if (abs(dx) < EPSILON)
		return -1.f;

	// calc the inverse determinant
	float idx = 1.f / dx;

	float3 rayToTri = ray.origin - tri.verts[0].position;

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

bool rayBoxCollision(Ray ray, Box box)
{
	float3 deltaMin = (box.bbMin - ray.origin) * ray.invDirection;
	float3 deltaMax = (box.bbMax - ray.origin) * ray.invDirection;

	float3 minVector = min(deltaMin, deltaMax);
	float3 maxVector = max(deltaMin, deltaMax);

	float minVal = max(max(minVector.x, minVector.y), minVector.z);
	float maxVal = min(min(maxVector.x, maxVector.y), maxVector.z);

	return 0 <= maxVal && minVal <= maxVal;
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

	int index;

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
			for (uint i = 0; i < 3; i++)
				triTmp.verts[i] = getUpdateVerts(indices[index + i]);

			// run collission test
			distance = rayTriangleCollision(ray, triTmp);

			//  update the triangle
			if (distance != -1 && (!colTri.collision || distance < colTri.distance))
			{
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
		leftChildCollision = rayBoxCollision(ray, BVHTree[leftChildNode].bbox);
		rightChildCollision = rayBoxCollision(ray, BVHTree[rightChildNode].bbox);

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

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	// get global index ID
	uint gloablIndexID = threadID.y * screenWidth + threadID.x;

	Ray ray;
	ColTri colTri;

	uint halfWidth = screenWidth >> 1;
	uint halfHeight = screenHeight >> 1;

	// check if this pixel is within bounds
	if (threadID.x < screenWidth && threadID.y < screenHeight)
	{
		// get the ray positioon
		ray.origin = float3(((float)threadID.x - halfWidth),
			((float)threadID.y - halfHeight), 0);

		// get the ray direction
		ray.direction = float3(0, 0, 1);

		// get the inverse of the direction
		ray.invDirection = 1.f / ray.direction;

		// find the collision with a triangle
		findCollision(ray, colTri);
		
		if (colTri.collision)
		{
			outputTex[gloablIndexID] = float4(colTri.distance / 100.f, 
				colTri.distance / 100.f,
				colTri.distance / 100.f,
				1);
		}
		else
		{
			outputTex[gloablIndexID] = float4(1, 0, 0, 1);
		}
	}
}