#ifndef HELPER_H
#define HELPER_H

#include <DirectXMath.h>

inline bool operator== (const DirectX::XMFLOAT2 &a, const DirectX::XMFLOAT2 &b)
{
	return a.x == b.x && a.y == b.y;
}

inline bool operator== (const DirectX::XMFLOAT3 &a, const DirectX::XMFLOAT3 &b)
{
	return a.x == b.x && a.y == b.y && a.z == a.z;
}

inline bool operator== (const DirectX::XMFLOAT4 &a, const DirectX::XMFLOAT4 &b)
{
	return a.x == b.x && a.y == b.y && a.z == a.z && a.w == b.w;
}

struct XMFLOAT3Compare
{
	bool operator() (const DirectX::XMFLOAT3 &a, const DirectX::XMFLOAT3 &b)
	{
		if (a.x < b.x)
			return true;
		else if (a.x > b.x)
			return false;
		// must be equal check y value

		if (a.y < b.y)
			return true;
		else if (a.y > b.y)
			return false;
		// must be equal check z value
		if (a.z < b.z)
			return true;

		return false;
	}
};

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}

#endif