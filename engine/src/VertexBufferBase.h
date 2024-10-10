#pragma once

#include "dataTypes.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Color.h"

//
enum VertexChannelIndices
{
	kIndex_Position,
	kIndex_Texcoord,
	kIndex_Color,
	kIndex_Normal,
	kIndex_CharacterAnimation_BoneIndices,
	kIndex_CharacterAnimation_BoneWeights,

	kChannel_Count
};

enum VertexChannelFlags
{
	kChannel_Position						= 0,							//	position data should always exist
	kChannel_Texcoord						= 1 << (kIndex_Texcoord - 1),
	kChannel_Color							= 1 << (kIndex_Color - 1),
	kChannel_Normal							= 1 << (kIndex_Normal - 1),
	kChannel_CharacterAnimation_BoneIndices	= 1 << (kIndex_CharacterAnimation_BoneIndices - 1),
	kChannel_CharacterAnimation_BoneWeights	= 1 << (kIndex_CharacterAnimation_BoneWeights - 1)
};

enum
{
	kNumFloatsPerPosition = 3,
	kNumFloatsPerTexcoord = 2,
	kNumFloatsPerColor = 4,
	kNumFloatsPerNormal = 3,
	kNumFloatsPerCharAnimBoneIdx = 4,
	kNumFloatsPerCharAnimBoneWeight = 4,
};

extern const int	s_channelSizes[];
inline int GetChannelSize(int channelIndex){ return s_channelSizes[channelIndex]; }

inline int GetVertSize(int vertFlags)
{
	int	returnValue = kNumFloatsPerPosition;
	for(int i = 1; i < kChannel_Count; i++)
	{
		if( vertFlags & (1 << (i - 1)) )
		{
			returnValue += GetChannelSize(i);
		}
	}
	
	returnValue *= sizeof(f32);
	return returnValue;
}

//	
class VertexBufferBase
{
public:
	void *GetData(){ return m_pData; }
	int GetType(){ return m_vertType; }
	u32 GetCount(){ return m_vertCount; }
	u32 GetBufferIndex(){ return m_bufferIndex; }

public:
	void		*m_pData;
	u32			m_vertCount;
	int			m_vertType;		//	from the VertexChannelFlags set
	u32			m_bufferIndex;
};

//	
class IndexBufferBase
{
public:
	u32 GetBufferIndex()
	{ 
		return m_bufferIndex;
	}

	u32 GetCount()
	{
		return m_indexCount;
	}

public:
	u32	*m_pIndices;
	u32	m_indexCount;
	u32	m_bufferIndex;
};