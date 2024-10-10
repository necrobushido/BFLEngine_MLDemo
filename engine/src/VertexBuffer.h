#pragma once

#include "types.h"
#include "VertexBufferBase.h"

enum eBufferType
{
	eBufferType_Static = GL_STATIC_DRAW,
	eBufferType_Dynamic = GL_DYNAMIC_DRAW
};

template <class dataType>
class VertexBuffer : public VertexBufferBase
{
public:
	VertexBuffer()
	{
		m_pData = NULL;
		m_vertCount = 0;
		m_bufferIndex = 0;

		glGenBuffers(1, &m_bufferIndex);
	}

	~VertexBuffer()
	{
		delete [] static_cast<dataType*>(m_pData);

		glDeleteBuffers(1, &m_bufferIndex);
	}

public:
	void Init(u32 vertCount)
	{
		dataType	*newData = new dataType[vertCount];
		m_pData = newData;
		m_vertCount = vertCount;
	}

	void ReInit(u32 newVertCount)
	{
		if( m_pData == NULL )
		{
			Init(newVertCount);
			return;
		}

		if( m_vertCount == newVertCount )
		{
			return;
		}

		dataType	*newData = new dataType[newVertCount];

		if( m_vertCount < newVertCount )
		{
			memcpy(newData, m_pData, m_vertCount * sizeof(dataType));
		}
		else
		{
			memcpy(newData, m_pData, newVertCount * sizeof(dataType));
		}

		delete [] static_cast<dataType*>(m_pData);

		m_pData = newData;
		m_vertCount = newVertCount;
	}

	dataType *BeginEdit()
	{
		return static_cast<dataType*>(m_pData);
	}

	void EndEdit(eBufferType bufferType = eBufferType_Static)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_bufferIndex);
		glBufferData(GL_ARRAY_BUFFER, m_vertCount * sizeof(dataType), m_pData, bufferType);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}	
};

class IndexBuffer : public IndexBufferBase
{
public:
	IndexBuffer()
	{
		m_pIndices = NULL;
		m_indexCount = 0;
		glGenBuffers(1, &m_bufferIndex);
	}

	~IndexBuffer()
	{
		delete [] m_pIndices;

		glDeleteBuffers(1, &m_bufferIndex);
	}

public:
	void Init(u32 indexCount)
	{
		m_indexCount = indexCount;
		m_pIndices = new u32[indexCount];
	}

	void ReInit(u32 newIndexCount)
	{
		if( m_pIndices == NULL )
		{
			Init(newIndexCount);
			return;
		}

		if( m_indexCount == newIndexCount )
		{
			return;
		}

		u32	*newIndices = new u32[newIndexCount];

		if( m_indexCount < newIndexCount )
		{
			memcpy(newIndices, m_pIndices, m_indexCount * sizeof(u32));
		}
		else
		{
			memcpy(newIndices, m_pIndices, newIndexCount * sizeof(u32));
		}

		delete [] m_pIndices;

		m_pIndices = newIndices;
		m_indexCount = newIndexCount;
	}

	u32 *BeginEdit()
	{
		return m_pIndices;
	}

	void EndEdit(eBufferType bufferType = eBufferType_Static)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferIndex);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexCount * sizeof(u32), m_pIndices, bufferType);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
};