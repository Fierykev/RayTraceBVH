#ifndef OBJECT_FILE_LOADER_H
#define OBJECT_FILE_LOADER_H

#include <wrl.h>
#include <DirectXMath.h>
#include "Helper.h"

// loading files

#include <fstream>

// get line in file

#include <sstream>

// Need these headers to support the array types I want

#include <map>
#include <vector>

#include <string>
#include <stdint.h>

// namespace time

using namespace std;// load all std:: things
using namespace DirectX;
using namespace Microsoft::WRL;

struct Material
{
	std::string name;

	XMFLOAT4 ambient;

	XMFLOAT4 diffuse;

	XMFLOAT4 specular;

	int shininess;

	float alpha;

	bool specularb;

	std::string texture_path;
};

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 texcoord;
};

struct VertexDataforMap
{
	XMFLOAT3 normal;
	XMFLOAT2 texcoord;
	unsigned int index;
};

struct CompareFLOAT3
{
	bool operator() (const DirectX::XMFLOAT3 &a, const DirectX::XMFLOAT3 &b) const
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

class ObjLoader
{
public:

	ObjLoader();

	~ObjLoader(); // destruction method

	HRESULT ObjLoader::Load(char *filename); // Load the object with its materials

																   // get the number of materials used

	const unsigned int ObjLoader::getMat_Num()
	{
		return material.size();
	}

	// get the material pointer

	Material* ObjLoader::getMaterials()
	{
		return &material.at(0);
	}

	// get the number of vertices in the object

	const unsigned int ObjLoader::getNumVertices()
	{
		return numVerts;
	}


	// get the number of indices in the object

	const unsigned int ObjLoader::getNumIndices()
	{
		//printf("%i\n", vx_array_i.size());
		return vx_array_i.size();
	}

	// get a pointer to the verticies

	const Vertex* ObjLoader::getVertices()
	{
		return vertex_final_array;
	}

	// get the vertex stride

	UINT ObjLoader::getVertexStride()
	{
		return sizeof(Vertex);
	}

	// get a pointer to the indices

	const unsigned int* ObjLoader::getIndices()
	{
		return &vx_array_i.at(0);
	}

	// get the number of meshes used to draw the object

	const unsigned int ObjLoader::getNumMesh()
	{
		return mesh_num;
	}

	// get a pointer to a certain material

	Material* ObjLoader::getMaterial(unsigned int mat_num)
	{
		return &material.at(mat_num);
	}

	void ObjLoader::freeOnStack();

private:

	// Create a vector to store the verticies

	void ObjLoader::Load_Geometry(char *filename); // load the verticies and indices

	void ObjLoader::Base_Mat(Material *mat); // the basic material

	vector <unsigned int> vx_array_i; // store the indecies for the vertex

	vector <float> vx_array; // store the verticies in the mesh

	Vertex* vertex_final_array; // the final verticies organized for Direct3D to draw

	vector <Material> material; // the materials used on the object

	vector <unsigned long> attributes;

	map <XMFLOAT3, vector<VertexDataforMap>, CompareFLOAT3> vertexmap; // map for removing doubles

	unsigned int numVerts;

	unsigned int mesh_num; // the number of meshes
};

#endif