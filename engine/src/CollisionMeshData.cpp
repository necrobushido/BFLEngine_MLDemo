#include "CollisionMeshData.h"

void CollisionMeshData::Init()
{
	vertBuffer.m_pData = (void*)((u64)vertBuffer.m_pData + (u64)this);
	indexBuffer.m_pIndices = (u32*)((u64)indexBuffer.m_pIndices + (u64)this);
}