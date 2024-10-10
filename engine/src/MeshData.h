#pragma once

#include "VertexBufferBase.h"
#include "DrawModes.h"

class MaterialData;

class MeshData
{
public:
	void Init();
	void Deinit();
	void Draw(const MaterialData *materialList) const;
	void DrawWireframe(const MaterialData *materialList) const;
	void DrawBase(const MaterialData *materialList) const;
	void PrepareForDraw();

	u32 GetSize()
	{	
		u32	returnValue = 0;
		returnValue += sizeof(MeshData);											//	base size
		returnValue += sizeof(u32) * indexBufferCount;								//	materialIndices
		returnValue += vertBuffer.m_vertCount * GetVertSize(vertBuffer.m_vertType);	//	vertex buffer data
		returnValue += sizeof(IndexBufferBase) * indexBufferCount;					//	index buffers
		for(u32 i = 0; i < indexBufferCount; i++)
		{
			returnValue += indexBuffers[i].m_indexCount * sizeof(u32);				//	index buffer data
		}

		return returnValue;
	}

public:
	VertexBufferBase	vertBuffer;
	IndexBufferBase		*indexBuffers;		//	indexed by indexBufferCount.  we have a separate index buffer for each material applied to the mesh
	u32					*materialIndices;	//	indices into the parent ModelData's "materials" array; indexed by indexBufferCount
	u32					indexBufferCount;
	eDrawMode			drawMode;
	u32					*vaos;				//	index by indexBufferCount.
};