#ifndef TEST_DATA_H
#define TEST_DATA_H

#include <DirectXMath.h>

using namespace DirectX;

typedef unsigned int uint;

struct char3
{
	char x, y, z;

	char3(char px, char py, char pz)
	{
		x = px;
		y = py;
		z = pz;
	}

	char3()
	{

	}

	char3(const char3 &data)
	{
		x = data.x;
		y = data.y;
		z = data.z;
	}
};

struct int2
{
	int x, y;

	int2(int px, int py)
	{
		x = px;
		y = py;
	}

	int2()
	{

	}

	int2(const int2 &data)
	{
		x = data.x;
		y = data.y;
	}
};

struct uint3
{
	uint x, y, z;

	uint3(uint px, uint py, uint pz)
	{
		x = px;
		y = py;
		z = pz;
	}

	uint3()
	{

	}

	uint3(const uint3 &data)
	{
		x = data.x;
		y = data.y;
		z = data.z;
	}

	uint& operator[](size_t index)
	{
		switch (index)
		{
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		}

		return x;
	}
};

struct float2
{
	float x, y;
	// TODO: CHECK ORDER
	float2(float px, float py)
	{
		x = px;
		y = py;
	}

	float2()
	{

	}

	float2(const float2 &data)
	{
		x = data.x;
		y = data.y;
	}
};

struct float3
{
	float x, y, z;
	// TODO: CHECK ORDER
	float3(float px, float py, float pz)
	{
		x = px;
		y = py;
		z = pz;
	}

	float3()
	{

	}

	float3(const float3 &data)
	{
		x = data.x;
		y = data.y;
		z = data.z;
	}

	float& operator[](size_t index)
	{
		switch (index)
		{
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		}

		return x;
	}
};

struct float4
{
	float x, y, z, w;
	// TODO: CHECK ORDER
	float4(float px, float py, float pz, float pw)
	{
		x = px;
		y = py;
		z = pz;
		w = pw;
	}

	float4(float3 p, float pw)
	{
		x = p.x;
		y = p.y;
		z = p.z;
		w = pw;
	}

	float4()
	{

	}

	float4(const float4 &data)
	{
		x = data.x;
		y = data.y;
		z = data.z;
		w = data.w;
	}

	float& operator[](size_t index)
	{
		switch (index)
		{
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		}

		return x;
	}
};

inline float3 operator-(const float3& a, const float3& b) {
	return float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline float3 operator*(const float3& a, const float3& b) {
	return float3(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline float3 operator/(const float& a, const float3& b) {
	return float3(a / b.x, a / b.y, a / b.z);
}

inline float3 operator/(const float3& a, const float3& b) {
	return float3(a.x / b.x, a.y / b.y, a.z / b.z);
}

inline float3 operator/=(const float3& b, const float& a) {
	return float3(b.x / a, b.y / a, b.z / a);
}

inline float3 operator+=(const float3& a, const float3& b) {
	return float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline uint3 operator+=(const uint3& a, const uint3& b) {
	return uint3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline float3 cross(const float3& a, const float3& b)
{
	return float3(a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

inline float dot(const float3& a, const float3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float3 mul(const float4& a, XMMATRIX* worldViewProjection)
{
	XMVECTOR vect{a.x, a.y, a.z, a.w};

	vect = XMVector4Transform(vect, *worldViewProjection);
	//return float3(a.x, a.y, a.z);
	return  float3(vect.m128_f32[0], vect.m128_f32[1], vect.m128_f32[2]);
}

// shapes

struct Box
{
	float3 bbMin, bbMax;

	Box()
	{

	}

	Box(const Box &data)
	{
		bbMin = data.bbMin;
		bbMax = data.bbMax;
	}
};

struct Ray
{
	float3 origin;
	float3 direction;

	float3 invDirection;

	Ray()
	{

	}

	Ray(const Ray &data)
	{
		origin = data.origin;
		direction = data.direction;
		invDirection = data.invDirection;
	}
};

// buffer structs

struct DebugNode
{
	int parent;
	int childL, childR;

	uint code;

	// bounding box calc
	Box bbox;

	// index start value
	uint index;

	DebugNode()
	{

	}

	DebugNode(const DebugNode &data)
	{
		parent = data.parent;
		childL = data.childL;
		childR = data.childR;
		code = data.code;
		bbox = data.bbox;
		index = data.index;
	}
};

struct VERTEX
{
	float3 position;
	float3 normal;
	float2 texcoord;

	VERTEX()
	{

	}

	VERTEX(const VERTEX &data)
	{
		position = data.position;
		normal = data.normal;
		texcoord = data.texcoord;
	}
};

struct Triangle
{
	VERTEX verts[3];

	Triangle()
	{

	}

	Triangle(const Triangle &data)
	{
		verts[0] = data.verts[0];
		verts[1] = data.verts[1];
		verts[2] = data.verts[2];
	}
};

struct ColTri
{
	bool collision;
	float distance;
	Triangle tri;

	ColTri()
	{

	}

	ColTri(const ColTri &data)
	{
		collision = data.collision;
		distance = data.distance;
		tri = data.tri;
	}
};

struct LeadingPrefixRet
{
	int num;
	uint index;
};

DebugNode* constructDebugTree(VERTEX* pverts, uint* pindices, uint pnumIndices,
	XMMATRIX* pworld, XMMATRIX* pworldViewProjection,
	uint screenWidth, uint screenHeight);

#endif