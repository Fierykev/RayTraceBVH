#ifndef OBJECT_FILE_LOADER_H
#define OBJECT_FILE_LOADER_H

#include <d3d12.h>
#include "d3dx12.h"
#include <wrl.h>
#include <DirectXMath.h>
#include "Helper.h"
#include "Image.h"

// loading files

#include <fstream>

// get line in file

#include <sstream>

// Need these headers to support the array types I want

#include <unordered_map>
#include <vector>

#include <string>
#include <stdint.h>

// namespace time

using namespace std;// load all std:: things
using namespace DirectX;
using namespace Microsoft::WRL;

namespace std
{
	template <>
	struct equal_to<DirectX::XMFLOAT3> : public unary_function<DirectX::XMFLOAT3, bool>
	{
		bool operator() (const DirectX::XMFLOAT3 &a, const DirectX::XMFLOAT3 &b) const
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		}
	};

	template<>
	struct hash<DirectX::XMFLOAT3> : public unary_function<DirectX::XMFLOAT3, size_t>
	{
		std::size_t operator() (const DirectX::XMFLOAT3& a) const
		{
			return std::hash<float>{}(a.x) ^ std::hash<float>{}(a.y) ^ std::hash<float>{}(a.z);
		}
	};
};

struct Material
{
	std::string name;
	
	XMFLOAT4 ambient;

	XMFLOAT4 diffuse;

	XMFLOAT4 specular;

	float shininess;

	float opticalDensity;

	float alpha;

	bool specularb;

	std::string texture_path;

	// DirectX Specific

	Image image;
};

struct MaterialUpload
{
	XMFLOAT4 ambient;

	XMFLOAT4 diffuse;

	XMFLOAT4 specular;

	float shininess;

	float opticalDensity;

	float alpha;

	bool specularb;

	int texNum;
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

class ObjLoader
{
public:

	ObjLoader();

	~ObjLoader(); // destruction method

	void Load(char *filename, ID3D12Device* Device); // Load the object with its materials
																   // get the number of materials used

	void UploadData(ID3D12GraphicsCommandList* commandList);

	const size_t ObjLoader::getMat_Num()
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

	const size_t ObjLoader::getNumIndices()
	{
		return vx_array_i.size();
	}

	// get the number of material indices in the object

	const size_t ObjLoader::getNumMaterialIndices()
	{
		return attributes.size();
	}

	// get the number of materials in the object

	const size_t ObjLoader::getNumMaterials()
	{
		return material.size();
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

	// get a pointer to the mapped index data

	ID3D12Resource* ObjLoader::getIndexMappedBuffer()
	{
		return mesh_indices.Get();
	}

	// get a pointer to the mapped vert data

	ID3D12Resource* ObjLoader::getVertexMappedBuffer()
	{
		return mesh_verts.Get();
	}

	// get a pointer to the mapped material data

	ID3D12Resource* ObjLoader::getMaterialIndexMappedBuffer()
	{
		return mesh_material_index.Get();
	}

	// get a pointer to the mapped index data

	ID3D12Resource* ObjLoader::getMaterialBuffer()
	{
		return mesh_material.Get();
	}

	// get the format of the indices

	DXGI_FORMAT ObjLoader::getIndexFormat()
	{
		return DXGI_FORMAT_R32_UINT;
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

	// get the primative type

	D3D12_PRIMITIVE_TOPOLOGY ObjLoader::getPrimativeType()
	{
		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

private:

	// Create a vector to store the verticies

	void Load_Geometry(char *filename, ID3D12Device* Device); // load the verticies and indices

	void Material_File(string filename, string matfile, unsigned long* tex_num, ID3D12Device* Device); // load the material file

	void Base_Mat(Material *mat); // the basic material

	vector <unsigned int> vx_array_i; // store the indecies for the vertex

	vector <float> vx_array; // store the verticies in the mesh

	Vertex* vertex_final_array = nullptr; // the final verticies organized for Direct3D to draw

	MaterialUpload* material_final_array = nullptr; // the final materials organized for Direct3D upload

	vector <Material> material; // the materials used on the object

	vector <unsigned int> attributes;

	unordered_map <XMFLOAT3, vector<VertexDataforMap>> vertexmap; // map for removing doubles

	unsigned int numVerts;

	// Mesh management

	ComPtr<ID3D12Resource> mesh_verts, mesh_verts_upload;

	ComPtr<ID3D12Resource> mesh_indices, mesh_indices_upload;

	ComPtr<ID3D12Resource> mesh_material_index, mesh_material_index_upload;

	ComPtr<ID3D12Resource> mesh_material, mesh_material_upload;

	unsigned int mesh_num; // the number of meshes
};

#endif