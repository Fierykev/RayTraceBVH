#include "ObjectFileLoader.h" // link to the header
#include "Helper.h"

/***************************************************************************
OBJ Loading
***************************************************************************/

ObjLoader::ObjLoader()
{
	vertex_final_array = nullptr;
}

void ObjLoader::freeOnStack()
{
	if (vertex_final_array != nullptr)
		free(vertex_final_array);
}

ObjLoader::~ObjLoader()
{
	// delete all data

	freeOnStack();
	const int ERROR_VALUE = 1;

	/*
	for (unsigned int i = 0; i < material->size(); i++)
	{
	Material *pMaterial = material.at(i);

	if (pMaterial->pTextureRV10 && pMaterial->pTextureRV10 == (ID3D10ShaderResourceView*)ERROR_VALUE)
	{
	ID3D10Resource* pRes = NULL;

	pMaterial->pTextureRV10->GetResource( &pRes );
	SAFE_RELEASE( pRes );
	SAFE_RELEASE( pRes );   // do this twice, because GetResource adds a ref

	SAFE_RELEASE( pMaterial->pTextureRV10 );
	}

	SAFE_DELETE( pMaterial );
	}*/

	// deallocate all of the vectors

	//delete vx_array_i;
	/*
	delete vx_array;

	delete vertex_final_array;

	delete material;

	delete attributes;

	delete mesh_num;

	SAFE_DELETE_ARRAY(m_pAttribTable);



	SAFE_RELEASE(mesh);*/
}

void ObjLoader::Base_Mat(Material *mat)
{
	ZeroMemory(mat, sizeof(mat)); // allocate memory

	mat->ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.f);
	mat->diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.f);
	mat->specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.f);
	mat->shininess = 0;
	mat->alpha = 1.0f;
	mat->specularb = false;
}

void ObjLoader::Load_Geometry(char *filename)
{
	// delete past memory

	freeOnStack();

	// allocate memory to the vectors on the heap

	vx_array_i.clear();

	vx_array.clear();

	material.clear();

	attributes.clear();

	mesh_num = 0;

	// create maps to store the lighting values for the material

	ifstream objFile(filename); // open the object file

	if (objFile.is_open()) // If the obj file is open, continue
	{
		// initialize the strings needed to read the file

		string line;

		string mat;

		// the material that is used

		unsigned long material_num = 0;

		unsigned long tex_num = 0;

		numVerts = 0;

		// Store the coordinates

		vector <float> vn_array;

		vector <float> vt_array;

		while (!objFile.eof()) // start reading file data
		{
			getline(objFile, line);	// get line from file

									// convert to a char to do pointers

			const char* ptr = line.c_str();

			if (ptr[0] == 'm' && ptr[1] == 't' && ptr[2] == 'l' && ptr[3] == 'l'  && ptr[4] == 'i' && ptr[5] == 'b' && ptr[6] == ' ') // load the material file
			{
				ptr += 7; // move the address up
			}
			if (ptr[0] == 'v' && ptr[1] == ' ') // the first character is a v: on this line is a vertex stored.
			{
				ptr += 2; // move address up

						  // store the three tmp's into the verticies

				float tmp[3];

				sscanf_s(ptr, "%f %f %f ", // read floats from the line: X Y Z
					&tmp[0],
					&tmp[1],
					&tmp[2]);

				vx_array.push_back(tmp[0]);
				vx_array.push_back(tmp[1]);
				vx_array.push_back(tmp[2]);
			}

			else if (ptr[0] == 'v' && ptr[1] == 'n') // the vertex normal
			{
				ptr += 2;

				// store the three tmp's into the verticies

				float tmp[3];

				sscanf_s(ptr, "%f %f %f ", // read floats from the line: X Y Z
					&tmp[0],
					&tmp[1],
					&tmp[2]);

				vn_array.push_back(tmp[0]);
				vn_array.push_back(tmp[1]);
				vn_array.push_back(tmp[2]);
			}

			else if (ptr[0] == 'v' && ptr[1] == 't') // texture coordinate for a vertex
			{
				ptr += 2;

				// store the two tmp's into the verticies

				float tmp[2];

				sscanf_s(ptr, "%f %f ",	// read floats from the line: X Y Z
					&tmp[0],
					&tmp[1]);

				vt_array.push_back(tmp[0]);
				vt_array.push_back(tmp[1]);
			}
			else if (ptr[0] == 'u' && ptr[1] == 's' && ptr[2] == 'e' && ptr[3] == 'm' && ptr[4] == 't' && ptr[5] == 'l') // which material is being used
			{

			}
			else if (ptr[0] == 'f') // store the faces in the object
			{
				ptr++;

				int vertexNumber[3] = { 0, 0, 0 };
				int normalNumber[3] = { 0, 0, 0 };
				int textureNumber[3] = { 0, 0, 0 };

				sscanf_s(ptr, "%i/%i/%i %i/%i/%i %i/%i/%i ",
					&vertexNumber[0],
					&textureNumber[0],
					&normalNumber[0],
					&vertexNumber[1],
					&textureNumber[1],
					&normalNumber[1],
					&vertexNumber[2],
					&textureNumber[2],
					&normalNumber[2]
					); // each point represents an X,Y,Z.

					   // create a vertex for this area

				for (int i = 0; 3 > i; i++) // loop for each triangle
				{
					Vertex vert;

					vert.position = XMFLOAT3(vx_array.at((vertexNumber[i] - 1) * 3), vx_array.at((vertexNumber[i] - 1) * 3 + 1), vx_array.at((vertexNumber[i] - 1) * 3 + 2));

					vert.normal = XMFLOAT3(vn_array[(normalNumber[i] - 1) * 3], vn_array[(normalNumber[i] - 1) * 3 + 1], vn_array[(normalNumber[i] - 1) * 3 + 2]);

					vert.texcoord = XMFLOAT2(vt_array[(textureNumber[i] - 1) * 2], vt_array[(textureNumber[i] - 1) * 2 + 1]);

					unsigned int index = 0;

					bool indexupdate = false;

					if (vertexmap.find(vert.position) != vertexmap.end())
						for (VertexDataforMap vdm : vertexmap[vert.position])
						{
							if (vert.normal == vdm.normal && vert.texcoord == vdm.texcoord) // found the index
							{
								index = vdm.index;

								indexupdate = true;
								break;
							}
						}

					// nothing found

					if (!indexupdate)
					{
						VertexDataforMap tmp;

						index = numVerts;
						tmp.normal = vert.normal;

						tmp.texcoord = vert.texcoord;

						tmp.index = index;

						vertexmap[vert.position].push_back(tmp);

						numVerts++;
					}

					vx_array_i.push_back(index);
				}

				// add the texture number to the attributes

				attributes.push_back(material_num);
			}
		}

		// create the final verts

		Vertex vert;

		vertex_final_array = new Vertex[numVerts];

		for (map<XMFLOAT3, vector<VertexDataforMap>>::iterator i = vertexmap.begin(); i != vertexmap.end(); i++)
		{
			for (VertexDataforMap vdm : i->second)
			{
				vertex_final_array[vdm.index].position = i->first;

				vertex_final_array[vdm.index].normal = vdm.normal;

				vertex_final_array[vdm.index].texcoord = vdm.texcoord;
			}
		}
	}
	else
	{
		printf("Error (OBJECT LOADER):  Cannot Find Object File- %s\n", filename);
	}
}

HRESULT ObjLoader::Load(char *filename)
{
	HRESULT hr;

	Load_Geometry(filename);

	float maxV = 0;

	for (int i = 0; i < getNumVertices(); i++)
		maxV = max(vertex_final_array[i].position.x, maxV);
	//2422
	for (int i = 0; i < getNumVertices(); i++)
		if (maxV == vertex_final_array[i].position.x)
			printf("%i\n", i);

	printf("%f\n", maxV);
	printf("%i\n", vx_array_i[3]);

	return S_OK;
}