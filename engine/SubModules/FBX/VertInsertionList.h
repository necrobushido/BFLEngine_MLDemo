#pragma once

#include "stdafx.h"
#include "IndexList.h"

class VertInsertionList
{
public:
	class Node
	{
	public:
		Node(u32 vertSize)
		{
			m_vertData = new u8[vertSize];
			m_pNext = NULL;
		}

		~Node()
		{
			delete [] m_vertData;

			//	done below in VertInsertionList destructor now
			//delete m_pNext;
		}

	public:
		u8			*m_vertData;
		IndexList	m_controlPointIndices;	//	control points associated with this vert
		Node		*m_pNext;
	};

public:
	VertInsertionList(u32 vertSize):
		m_pRootNode(NULL),
		m_pLastNode(NULL),
		m_vertSize(vertSize),
		m_vertCount(0)
	{
	}

	~VertInsertionList()
	{
		//	done this way instead of recursively in order to avoid blowing out the stack
		Node*	pNodeToDelete = m_pRootNode;
		while(m_pRootNode)
		{
			pNodeToDelete = m_pRootNode;
			m_pRootNode = m_pRootNode->m_pNext;
			delete pNodeToDelete;
		}
	}

public:
	Node *GetVertNode(u32 index)
	{
		u32		currentIndex = 0;
		Node	*currentNode = m_pRootNode;
		while(currentNode &&
			currentIndex < index )
		{
			currentNode = currentNode->m_pNext;
			currentIndex++;
		}

		return currentNode;
	}

	u32 Insert(u8 *vertData, u32 originalControlPointIndex)
	{
		u32	returnValue = 0;
		if( m_pRootNode == NULL )
		{
			m_pRootNode = new Node(m_vertSize);
			m_pLastNode = m_pRootNode;
			memcpy(m_pRootNode->m_vertData, vertData, m_vertSize);
			m_pRootNode->m_controlPointIndices.Insert(originalControlPointIndex, true);
			returnValue = 0;
			m_vertCount++;
		}
		else
		{
			bool	found = false;
			int		index = 0;
			Node	*currentNode = m_pRootNode;
			while(	currentNode && 
					!found)
			{
				//	check to see if the vert is a dupe
				if( memcmp(currentNode->m_vertData, vertData, m_vertSize) == 0 )
				{
					//	if it is, don't insert, just assign it the current index
					returnValue = index;
					currentNode->m_controlPointIndices.Insert(originalControlPointIndex, true);
					found = true;
				}

				index++;
				currentNode = currentNode->m_pNext;
			}

			//	if this vert wasn't a dupe, insert it into the list
			if( !found )
			{
				m_pLastNode->m_pNext = new Node(m_vertSize);
				m_pLastNode = m_pLastNode->m_pNext;
				memcpy(m_pLastNode->m_vertData, vertData, m_vertSize);
				m_pLastNode->m_controlPointIndices.Insert(originalControlPointIndex, true);
				returnValue = index;
				m_vertCount++;
			}
		}

		return returnValue;
	}

public:
	Node	*m_pRootNode;
	Node	*m_pLastNode;
	u32		m_vertSize;
	u32		m_vertCount;
};