#include "types.h"

#include "ModelData.h"
#include "Renderer.h"
#include "DefaultShaderProgram.h"
#include "Texture.h"
#include "FileManager.h"

void ModelData::Init()
{
	if( collisionMesh != NULL )
	{
		//DebugPrintf("\tModel : %d meshes, %d materials, %d bones, has collision\n", meshCount, materialCount, boneCount);
	}
	else
	{
		//DebugPrintf("\tModel : %d meshes, %d materials, %d bones, no collision\n", meshCount, materialCount, boneCount);
	}

	if( materialCount > 0 )
	{
		materials = (MaterialData*)((u64)materials + (u64)this);
	}

	if( boneCount > 0 )
	{
		bones = (BoneData*)((u64)bones + (u64)this);
	}

	if( collisionMesh != NULL )
	{
		collisionMesh = (CollisionMeshData*)((u64)collisionMesh + (u64)this);
		collisionMesh->Init();
	}

	meshPtrs = (MeshData**)((u64)meshPtrs + (u64)this);

	for(int i = 0; i < meshCount; i++)
	{
		meshPtrs[i] = (MeshData*)((u64)meshPtrs[i] + (u64)this);
	}

	for(int i = 0; i < meshCount; i++)
	{
		meshPtrs[i]->Init();
	}

	//	load textures
	for(u32 i = 0; i < materialCount; i++)
	{
		for(int channelIdx = 0; channelIdx < MaterialData::kMaterialChannelCount; channelIdx++)
		{
			if( materials[i].channels[channelIdx].texture[0] != '\0' )
			{
				if( g_fileManager->FileExists(materials[i].channels[channelIdx].texture) )
				{
					materials[i].channels[channelIdx].textureRef = Texture::MakeRef(materials[i].channels[channelIdx].texture);
					//DebugPrintf("\t\ttexture %s loaded\n", materials[i].channels[channelIdx].texture);
				}
				else
				{
					const char	*kDefaultTexture = "Blank.tga";
					//DebugPrintf("\t\twarning! : texture %s does not exist.  loading %s instead.\n", materials[i].channels[channelIdx].texture, kDefaultTexture);
					materials[i].channels[channelIdx].textureRef = Texture::MakeRef(kDefaultTexture);
				}
			}
			else
			{
				const char	*kDefaultTexture = "Blank.tga";
				materials[i].channels[channelIdx].textureRef = Texture::MakeRef(kDefaultTexture);
			}
		}
	}

	//DebugPrintf("initializing bones:\n");
	//for(u32 i = 0; i < boneCount; i++)
	//{
		//DebugPrintf("\tbone %d name = %s\n", i, bones[i].name);
	//}
}

void ModelData::Deinit()
{
	for(int i = 0; i < meshCount; i++)
	{
		meshPtrs[i]->Deinit();
	}
}

void ModelData::Deallocate()
{
	for(int i = 0; i < meshCount; i++)
	{
		delete [] meshPtrs[i]->vertBuffer.m_pData;
		for(u32 j = 0; j < meshPtrs[i]->indexBufferCount; j++)
		{
			delete [] meshPtrs[i]->indexBuffers[j].m_pIndices;
		}
		delete [] meshPtrs[i]->indexBuffers;
		delete [] meshPtrs[i]->materialIndices;
		delete meshPtrs[i];
	}

	delete [] bones;
	delete [] materials;
	delete [] meshPtrs;
}

void ModelData::Draw() const
{
	DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();

	for(int i = 0; i < meshCount; i++)
	{
		meshPtrs[i]->Draw(materials);
	}
}

void ModelData::DrawWireframe() const
{
	DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();

	for(int i = 0; i < meshCount; i++)
	{
		meshPtrs[i]->DrawWireframe(materials);
	}
}

void ModelData::DrawBase() const
{
	for(int i = 0; i < meshCount; i++)
	{
		meshPtrs[i]->DrawBase(materials);
	}
}