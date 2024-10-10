#pragma once

#include "VertexBufferBase.h"

class CollisionMeshData
{
public:
	void Init();

	u32 GetSize()
	{	
		u32	returnValue = 0;
		returnValue += sizeof(CollisionMeshData);									//	base size
		returnValue += vertBuffer.m_vertCount * GetVertSize(kChannel_Position);		//	vertex buffer data
		returnValue += indexBuffer.m_indexCount * sizeof(u32);						//	index buffer data

		return returnValue;
	}

public:
	VertexBufferBase	vertBuffer;
	IndexBufferBase		indexBuffer;
};