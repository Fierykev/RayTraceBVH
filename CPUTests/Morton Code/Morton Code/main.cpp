#include <algorithm>
#include <bitset>
#include <iostream>

using namespace std;

void outputUnsignedIntBits(const unsigned int val)
{
	std::bitset<32> x(val);
	std::cout << x << endl;
}

void outputFloatIntBits(const float val)
{
	union
	{
		float frep;
		int irep;
	} ftoi;

	ftoi.frep = val;

	std::bitset<32> x(*((unsigned int*)&val));
	std::cout << x << endl;

	std::bitset<32> y(ftoi.irep);
	std::cout << y << endl;
}


// Expands a 10-bit integer into 30 bits
// by inserting 2 zeros after each bit.
unsigned int expandBits(unsigned int v)
{
	v = (v * 0x00010001u) & 0xFF0000FFu;
	v = (v * 0x00000101u) & 0x0F00F00Fu;
	v = (v * 0x00000011u) & 0xC30C30C3u;
	v = (v * 0x00000005u) & 0x49249249u;
	return v;
}

// Calculates a 30-bit Morton code for the
// given 3D point located within the unit cube [0,1].
unsigned int morton3D(float x, float y, float z)
{
	x = min(max(x * 1024.0f, 0.0f), 1023.0f);
	y = min(max(y * 1024.0f, 0.0f), 1023.0f);
	z = min(max(z * 1024.0f, 0.0f), 1023.0f);
	unsigned int xx = expandBits((unsigned int)x);
	unsigned int yy = expandBits((unsigned int)y);
	unsigned int zz = expandBits((unsigned int)z);

	return xx * 4 + yy * 2 + zz;
}

unsigned int expand(unsigned int var)
{
	const unsigned int byteMasks[] = { 0x09249249, 0x030c30c3, 0x0300f00f, 0x030000ff, 0x000003ff };
	int curVal = 16;

	for (int i = 4; 0 < i; i--)
	{
		var &= byteMasks[i]; // set everything other than the last 10 bits to zero
		var |= var << curVal; // add in the byte offset

		// divide by two
		curVal /= 2;
	}

	var &= byteMasks[0];

	return var;
}

unsigned int calcMorton(float x, float y, float z)
{
	float p[] = { x, y, z };

	unsigned int code[3];

	// multiply out
	for (int i = 0; i < 3; i++)
	{
		// change to base .5
		p[i] *= 1024.f;

		if (p[i] < 0)
			p[i] = 0;
		else if (p[i] >= 1024)
			p[i] = 1023;

		code[i] = expand(p[i]);
	}
	//outputUnsignedIntBits(p[0]);
	//outputUnsignedIntBits(code[0]);
	// combine and return the code
	return code[2] | code[1] << 1 | code[0] << 2;
}

int main()
{
	//outputUnsignedIntBits(0x00010001u);
	outputUnsignedIntBits(morton3D(.625f, .4375f, .75f));
	outputUnsignedIntBits(calcMorton(.625f, .4375f, .75f));
	for (int i = sizeof(unsigned int) * 8; i >= 0; i--)
		printf("%i", i%10);
	system("PAUSE");

	return 0;
}