#include <RayTraceGlobal.hlsl>

#define STACK_SIZE 2<<5

/*
The slab method is used for collision.
*/

bool rayBoxCollision(Ray ray, Box box)
{
	float3 deltaMin = (box.bbMin - ray.position) * ray.invDirection;
	float3 deltaMax = (box.bbMax - ray.position) * ray.invDirection;

	float3 minVector = min(deltaMin, deltaMax);
	float3 maxVector = max(deltaMin, deltaMax);

	return minVector <= maxVector;
}

uint findCollision(Ray ray)
{
	// create the stack vars
	uint stackIndex = 0;
	uint stack[STACK_SIZE];

	// start from the root
	int nodeID = numObjects;

	int leftChildNode, rightChildNode;

	bool leftChildCollision, rightChildCollision;
	int leftLeaf, rightLeaf;

	do
	{
		// record the index of the children
		leftChildNode = BVHTree[nodeID].childL;
		rightChildNode = BVHTree[nodeID].childR;

		// check for leaf node
		if (leftChildNode == -1 && rightChildNode == -1)
		{
			// TODO: add to check list for triangle collision


			// remove the stack top
			nodeID = --stackIndex;

			continue;
		}

		// check for collisions with children
		leftChildCollision = rayBoxCollision(ray, BVHTree[leftChildNode].bbox);
		rightChildCollision = rayBoxCollision(ray, BVHTree[rightChildNode].bbox);

		if (!leftChildCollision && !rightChildCollision)
		{
			// remove the stack top
			nodeID = --stackIndex;
		}
		else
		{
			// store the right node on the stack if both right and left
			// boxes intersect the ray
			if (leftChildCollision && rightChildCollision)
				stack[stackIndex++] = rightChildNode;

			// update the nodeID depending on if the left side intersected
			nodeID = leftChildCollision ? leftChildNode : rightChildNode;
		}
	} while (nodeID != -1);
}

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{

}