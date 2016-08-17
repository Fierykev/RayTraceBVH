
#include "ObjectFileLoader.h" // link to the header
#include "Shader.h"
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
	mat->pTextureRV12 = NULL;
}

void ObjLoader::Material_File(string filename, string matfile, unsigned long* tex_num, ID3D12Device* Device)
{
	// find the directory to the material file

	string directory = filename.substr(0, filename.find_last_of('/') + 1);

	matfile = directory + matfile; // the location of the material file to the program

								   // open the file

	ifstream matFile_2(matfile);

	if (matFile_2.is_open()) // If obj file is open, continue
	{
		string line_material;// store each line of the file here

		while (!matFile_2.eof()) // Start reading file data as long as we have not reached the end
		{
			getline(matFile_2, line_material); // Get a line from file

											   // convert to a char to do pointer arithmetics

			char* ptr = (char*)line_material.c_str();

			if (ptr[0] == '	')// this is tab not space (3ds max uses tabs which would otherwise confuse this program without this line)
			{
				ptr++;// move address up
			}

			// This program is for standard Wavefront Objects that are triangulated and have normals stored in the file.  This reader has been tested with 3ds Max and Blender.

			if (ptr[0] == 'n' && ptr[1] == 'e' && ptr[2] == 'w' && ptr[3] == 'm'
				&& ptr[4] == 't' && ptr[5] == 'l') // new material
			{
				ptr += 6 + 1;// move address up

				Material mat; // allocate memory to create a new material

				Base_Mat(&mat); // init the material

				mat.name = ptr; // set the name of the material

				material.push_back(mat); // add to the vector

				*tex_num = material.size() - 1;
			}
			else if (ptr[0] == 'K' && ptr[1] == 'a') // ambient
			{
				ptr += 2;// move address up

				sscanf_s(ptr, "%f %f %f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).ambient.x,
					&material.at(*tex_num).ambient.y,
					&material.at(*tex_num).ambient.z);

				material.at(*tex_num).ambient.w = 1.f;
			}
			else if (ptr[0] == 'K' && ptr[1] == 'd') // diffuse
			{
				ptr += 2;// move address up

				sscanf_s(ptr, "%f %f %f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).diffuse.x,
					&material.at(*tex_num).diffuse.y,
					&material.at(*tex_num).diffuse.z);

				material.at(*tex_num).diffuse.w = 1.f;
			}
			else if (ptr[0] == 'K' && ptr[1] == 's') // specular
			{
				ptr += 2;// move address up

				sscanf_s(ptr, "%f %f %f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).specular.x,
					&material.at(*tex_num).specular.y,
					&material.at(*tex_num).specular.z);

				material.at(*tex_num).specular.w = 1.f;
			}
			else if (ptr[0] == 'N' && ptr[1] == 's') // shininess
			{
				ptr += 2;// move address up

				sscanf_s(ptr, "%f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).shininess);
			}
			else if (ptr[0] == 'd') // transparency
			{
				ptr++;// move address up

				sscanf_s(ptr, "%f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).alpha);
			}
			else if (ptr[0] == 'T' && ptr[0] == 'r') // another way to store transparency
			{
				ptr += 2;// move address up

				sscanf_s(ptr, "%f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).alpha);
			}
			else if (ptr[0] == 'm' && ptr[1] == 'a' && ptr[2] == 'p' && ptr[3] == '_'
				&& ptr[4] == 'K' && ptr[5] == 'd') // image texture
			{
				ptr += 7;// move address up

				material.at(*tex_num).texture_path = directory + ptr; // the material file

																	  // load the file

																	  // convert to a LPWSTR

				wstring wstr_;
				wstr_.assign(material.at(*tex_num).texture_path.begin(), material.at(*tex_num).texture_path.end());
				LPWSTR LPWSTR_ = const_cast<LPWSTR>(wstr_.c_str());

				// TODO: add texture support
				//if (FAILED(D3DX12CreateShaderResourceViewFromFile(Device, LPWSTR_, NULL, NULL, &material.at(*tex_num).pTextureRV12, NULL))) // create the texture
				//{
					//printf("Error (OBJECT LOADER): Cannot Load Image File- %s\n", material.at(*tex_num).texture_path.c_str());
				//}
			}
		}

		matFile_2.close(); // close the file
	}
	else
	{
		printf("Error (OBJECT LOADER): Cannot Find Material File- %s\n", matfile.c_str());;
	}
}

void ObjLoader::Load_Geometry(char *filename, ID3D12Device* Device)
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

				const string material_file = ptr;// the material file

				Material_File(filename, material_file, &tex_num, Device); // read the material file and update the number of materials
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
				mat = line.substr(6 + 1, line.length());// save so the comparison will work

														// add new to the material name so that it matches the names of the materials in the mtl file

				for (unsigned long num = 0; num < tex_num + 1; num++)// find the material
				{
					if (mat == material.at(num).name)// matches material in mtl file
					{
						material_num = num;
					}
				}
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

HRESULT ObjLoader::Load(char *filename, ID3D12Device* Device)
{
	HRESULT hr;

	Load_Geometry(filename, Device);

	// Load the materials
	const int ERROR_VALUE = 1;

	for (unsigned long i = 0; i < material.size(); i++)
	{
		Material *pMaterial = &material.at(i);
		if (pMaterial->texture_path[0] != '\0') // this array holds data
		{
			pMaterial->pTextureRV12 = (D3D12_SUBRESOURCE_DATA*)ERROR_VALUE; // it equals to an error value

																			  // TODO: Check if the file exists first
			{
				// convert string to WCHAR

				wstring wstr_;
				wstr_.assign(pMaterial->texture_path.begin(), pMaterial->texture_path.end());
				LPWSTR LPWSTR_ = const_cast<LPWSTR>(wstr_.c_str());

				// TODO: LOAD TEXTURE
				//DXUTGetGlobalResourceCache().CreateTextureFromFile(Device, Context, LPWSTR_, &pMaterial->pTextureRV11, false);
			}
		}
	}

	// Now let's place the object mesh into the buffers structure

	// Setup vertex buffer

	ThrowIfFailed(Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * getNumVertices()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mesh_verts)));

	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(mesh_verts->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, (void*)vertex_final_array, sizeof(Vertex) * getNumVertices());
	mesh_verts->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	vertex_buffer.BufferLocation = mesh_verts->GetGPUVirtualAddress();
	vertex_buffer.StrideInBytes = sizeof(Vertex);
	vertex_buffer.SizeInBytes = sizeof(Vertex) * getNumVertices();
	
	// Setup index buffer

	ThrowIfFailed(Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int) * getNumIndices()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mesh_indices)));

	UINT8* pIndexDataBegin;
	ThrowIfFailed(mesh_indices->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, (void*)&vx_array_i.at(0), sizeof(unsigned int) * getNumIndices());
	mesh_indices->Unmap(0, nullptr);

	// Initialize the index buffer view.
	index_buffer.BufferLocation = mesh_indices->GetGPUVirtualAddress();
	index_buffer.Format = getIndexFormat();
	index_buffer.SizeInBytes = sizeof(unsigned int) * getNumIndices();
/*	
	float maxV = 0;

	for (int i = 0; i < getNumVertices(); i++)
		maxV = max(vertex_final_array[i].position.z, maxV);
	//2422
	for (int i = 0; i < getNumVertices(); i++)
		if (maxV == vertex_final_array[i].position.z)
			printf("%i\n", i);

	printf("%f\n", maxV);
	printf("%i\n", vx_array_i[3]);

	printf("%f", vertex_final_array[2444].position.z);*/
	
	return S_OK;
}