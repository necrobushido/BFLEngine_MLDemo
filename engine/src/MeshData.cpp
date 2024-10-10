#include "types.h"

#include "MeshData.h"
#include "MaterialData.h"

#include "Renderer.h"
#include "DefaultShaderProgram.h"

void MeshData::Init()
{
	//DebugPrintf("\t\tinitializing Mesh : %d verts, %d index buffers\n", vertBuffer.m_vertCount, indexBufferCount);

	//	fixup the pointers
	materialIndices = (u32*)((u64)materialIndices + (u64)this);
	vertBuffer.m_pData = (void*)((u64)vertBuffer.m_pData + (u64)this);
	indexBuffers = (IndexBufferBase*)((u64)indexBuffers + (u64)this);

	for(u32 i = 0; i < indexBufferCount; i++)
	{
		indexBuffers[i].m_pIndices = (u32*)((u64)indexBuffers[i].m_pIndices + (u64)this);
	}

	PrepareForDraw();
}

void MeshData::PrepareForDraw()
{
	//	prepare vertex array
	glGenBuffers(1, &vertBuffer.m_bufferIndex);
	glBindBuffer(GL_ARRAY_BUFFER, vertBuffer.m_bufferIndex);
	glBufferData(GL_ARRAY_BUFFER, vertBuffer.m_vertCount * GetVertSize(vertBuffer.GetType()), vertBuffer.m_pData, GL_STATIC_DRAW);	

	//	prepare index arrays
	for(u32 i = 0; i < indexBufferCount; i++)
	{
		glGenBuffers(1, &indexBuffers[i].m_bufferIndex);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffers[i].m_bufferIndex);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffers[i].m_indexCount * sizeof(u32), indexBuffers[i].m_pIndices, GL_STATIC_DRAW);
	}

	//	prepare vaos
	vaos = new u32[indexBufferCount];
	for(u32 i = 0; i < indexBufferCount; i++)
	{
		glGenVertexArrays(1, &vaos[i]);
		glBindVertexArray(vaos[i]);

		glBindBuffer(GL_ARRAY_BUFFER, vertBuffer.m_bufferIndex);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffers[i].m_bufferIndex);
		Renderer::SetAttributes(&vertBuffer);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

void MeshData::Deinit()
{
	glDeleteBuffers(1, &vertBuffer.m_bufferIndex);
	for(u32 i = 0; i < indexBufferCount; i++)
	{
		glDeleteBuffers(1, &indexBuffers[i].m_bufferIndex);
		glDeleteVertexArrays(1, &vaos[i]);
	}

	delete [] vaos;
}

void MeshData::Draw(const MaterialData *materialList) const
{
	DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();

	pDefaultShaderProgram->EnableTexture(		vertBuffer.m_vertType & kChannel_Texcoord);
	pDefaultShaderProgram->EnableVertexColor(	vertBuffer.m_vertType & kChannel_Color);
	pDefaultShaderProgram->EnableLighting(		vertBuffer.m_vertType & kChannel_Normal);
	pDefaultShaderProgram->EnableSkinning(		vertBuffer.m_vertType & kChannel_CharacterAnimation_BoneIndices);

	for(u32 i = 0; i < indexBufferCount; i++)
	{
		const MaterialData	*ourMaterial = &materialList[materialIndices[i]];		

		for(int j = 0; j < MaterialData::kMaterialChannelCount; j++)
		{
			if( ourMaterial->channels[j].textureRef )
			{
				ourMaterial->channels[j].textureRef->Bind(j);
			}
		}

		Renderer::DrawBuffer(drawMode, indexBuffers[i].GetCount(), vaos[i]);
	}
}

void MeshData::DrawWireframe(const MaterialData *materialList) const
{
	DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();

	pDefaultShaderProgram->EnableTexture(		false);
	pDefaultShaderProgram->EnableVertexColor(	false);
	pDefaultShaderProgram->EnableLighting(		false);
	pDefaultShaderProgram->EnableSkinning(		vertBuffer.m_vertType & kChannel_CharacterAnimation_BoneIndices);

	for(u32 i = 0; i < indexBufferCount; i++)
	{
		const MaterialData	*ourMaterial = &materialList[materialIndices[i]];		

		for(int j = 0; j < MaterialData::kMaterialChannelCount; j++)
		{
			if( ourMaterial->channels[j].textureRef )
			{
				ourMaterial->channels[j].textureRef->Bind(j);
			}
		}

		Renderer::DrawBuffer(kLineLoop, indexBuffers[i].GetCount(), vaos[i]);
	}
}

void MeshData::DrawBase(const MaterialData *materialList) const
{
	for(u32 i = 0; i < indexBufferCount; i++)
	{
		const MaterialData	*ourMaterial = &materialList[materialIndices[i]];		

		for(int j = 0; j < MaterialData::kMaterialChannelCount; j++)
		{
			if( ourMaterial->channels[j].textureRef )
			{
				ourMaterial->channels[j].textureRef->Bind(j);
			}
		}

		Renderer::DrawBuffer(drawMode, indexBuffers[i].GetCount(), vaos[i]);
	}
}