#include "stdafx.h"

#include "WriteModel.h"

void WriteCollisionMesh(CollisionMeshData *mesh, FILE *pFile)
{
	intptr_t	currentOffset = sizeof(CollisionMeshData);
	int			sizeOfVert = GetVertSize(kChannel_Position);
	size_t		bytesWritten;

	//	write the struct
	void		*tempVerts = mesh->vertBuffer.m_pData;
	mesh->vertBuffer.m_pData = (void*)currentOffset;
	currentOffset += mesh->vertBuffer.m_vertCount * sizeOfVert;

	u32			*tempIndices = mesh->indexBuffer.m_pIndices;
	mesh->indexBuffer.m_pIndices = (u32*)currentOffset;
	currentOffset += mesh->indexBuffer.m_indexCount * sizeof(u32);

	bytesWritten = fwrite(mesh, 1, sizeof(CollisionMeshData), pFile);
	assert(bytesWritten == sizeof(CollisionMeshData));

	mesh->vertBuffer.m_pData = tempVerts;
	mesh->indexBuffer.m_pIndices = tempIndices;
	
	//	write the vertex data
	bytesWritten = fwrite(mesh->vertBuffer.m_pData, 1, mesh->vertBuffer.m_vertCount * sizeOfVert, pFile);
	assert(bytesWritten == mesh->vertBuffer.m_vertCount * sizeOfVert);

	//	write the indices
	bytesWritten = fwrite(mesh->indexBuffer.m_pIndices, 1, mesh->indexBuffer.m_indexCount * sizeof(u32), pFile);
	assert(bytesWritten == mesh->indexBuffer.m_indexCount * sizeof(u32));
}

void WriteMesh(MeshData *mesh, FILE *pFile)
{
	intptr_t	currentOffset = sizeof(MeshData);
	int			sizeOfVert = GetVertSize(mesh->vertBuffer.m_vertType);
	size_t		bytesWritten;

	//	write the MeshData struct
	u32			*tempMaterialIndices = mesh->materialIndices;
	mesh->materialIndices = (u32*)currentOffset;
	currentOffset += mesh->indexBufferCount * sizeof(u32);

	void		*tempVerts = mesh->vertBuffer.m_pData;
	mesh->vertBuffer.m_pData = (void*)currentOffset;
	currentOffset += mesh->vertBuffer.m_vertCount * sizeOfVert;

	IndexBufferBase	*tempIndexBuffers = mesh->indexBuffers;
	mesh->indexBuffers = (IndexBufferBase*)currentOffset;
	currentOffset += mesh->indexBufferCount * sizeof(IndexBufferBase);

	bytesWritten = fwrite(mesh, 1, sizeof(MeshData), pFile);
	assert(bytesWritten == sizeof(MeshData));

	mesh->vertBuffer.m_pData = tempVerts;
	mesh->indexBuffers = tempIndexBuffers;
	mesh->materialIndices = tempMaterialIndices;
	
	//	write the material indices
	bytesWritten = fwrite(mesh->materialIndices, 1, mesh->indexBufferCount * sizeof(u32), pFile);
	assert(bytesWritten == mesh->indexBufferCount * sizeof(u32));

	//	write the vertex data
	bytesWritten = fwrite(mesh->vertBuffer.m_pData, 1, mesh->vertBuffer.m_vertCount * sizeOfVert, pFile);
	assert(bytesWritten == mesh->vertBuffer.m_vertCount * sizeOfVert);

	//	write the index buffer structs
	u32			**tempIndices = new u32*[mesh->indexBufferCount];
	for(u32 j = 0; j < mesh->indexBufferCount; j++)
	{
		tempIndices[j] = mesh->indexBuffers[j].m_pIndices;
		mesh->indexBuffers[j].m_pIndices = (u32*)currentOffset;
		currentOffset += mesh->indexBuffers[j].m_indexCount * sizeof(u32);
	}
	
	bytesWritten = fwrite(mesh->indexBuffers, 1, mesh->indexBufferCount * sizeof(IndexBufferBase), pFile);
	assert(bytesWritten == mesh->indexBufferCount * sizeof(IndexBufferBase));
	
	for(u32 j = 0; j < mesh->indexBufferCount; j++)
	{
		mesh->indexBuffers[j].m_pIndices = tempIndices[j];
	}
	delete [] tempIndices;

	//	write the indices for the index buffers
	for(u32 j = 0; j < mesh->indexBufferCount; j++)
	{
		bytesWritten = fwrite(mesh->indexBuffers[j].m_pIndices, 1, mesh->indexBuffers[j].m_indexCount * sizeof(u32), pFile);
		assert(bytesWritten == mesh->indexBuffers[j].m_indexCount * sizeof(u32));
	}
}

//	when writing the model, we set the internal pointers equal to the offset into the file being written where the data they point to will be written later
//	then, at runtime, all we have to do after loading the file is add the origin pointer of the file data to the offset to find the actual data
//	TODO : need data alignment?
void WriteModel(ModelData *model, FILE *pFile)
{
	intptr_t	currentOffset = sizeof(ModelData);

	//	write the model
	MaterialData*	tempMaterials = model->materials;
	if( model->materialCount > 0 )
	{
		model->materials = (MaterialData*)currentOffset;
		currentOffset += sizeof(MaterialData) * model->materialCount;
	}	

	BoneData*	tempBones = model->bones;
	if( model->boneCount > 0 )
	{
		model->bones = (BoneData*)currentOffset;
		currentOffset += sizeof(BoneData) * model->boneCount;
	}

	CollisionMeshData*	tempCollisionMesh = model->collisionMesh;
	if( model->collisionMesh != NULL )
	{
		model->collisionMesh = (CollisionMeshData*)currentOffset;
		currentOffset += tempCollisionMesh->GetSize();
	}

	MeshData**	tempMeshPtrs = model->meshPtrs;
	model->meshPtrs = (MeshData**)currentOffset;
	currentOffset += sizeof(MeshData*) * model->meshCount;

	size_t	bytesWritten;
	bytesWritten = fwrite(model, 1, sizeof(ModelData), pFile);
	assert(bytesWritten == sizeof(ModelData));

	model->meshPtrs = tempMeshPtrs;
	model->bones = tempBones;
	model->materials = tempMaterials;
	model->collisionMesh = tempCollisionMesh;

	//	write the materials
	if( model->materialCount > 0 )
	{
		bytesWritten = fwrite(model->materials, 1, sizeof(MaterialData) * model->materialCount, pFile);
		assert(bytesWritten == sizeof(MaterialData) * model->materialCount);
	}

	//	write the bones
	if( model->boneCount > 0 )
	{
		bytesWritten = fwrite(model->bones, 1, sizeof(BoneData) * model->boneCount, pFile);
		assert(bytesWritten == sizeof(BoneData) * model->boneCount);
	}

	//	write the collision mesh
	if( model->collisionMesh != NULL )
	{
		WriteCollisionMesh(model->collisionMesh, pFile);
	}

	//	now write the mesh pointers
	for(int i = 0; i < model->meshCount; i++)
	{
		MeshData	*ptr = (MeshData*)currentOffset;
		bytesWritten = fwrite(&ptr, 1, sizeof(MeshData*), pFile);
		assert(bytesWritten == sizeof(MeshData*));

		currentOffset += model->meshPtrs[i]->GetSize();
	}

	//	now write the meshes themselves
	for(int i = 0; i < model->meshCount; i++)
	{
		WriteMesh(model->meshPtrs[i], pFile);
	}
}

void WriteModelToFile(ModelData &modelData, const char *filename)
{
	FILE	*pFile = fopen(filename, "wb");
	if( pFile == NULL )
	{
		printf("error opening file %s\n", filename);
		assert(0);
	}

	WriteModel(&modelData, pFile);

	fclose(pFile);
}