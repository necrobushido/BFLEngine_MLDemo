#pragma once

#include "stdafx.h"

class IndexList
{
public:
	class Node
	{
	public:
		Node()
		{
			m_pNext = NULL;
		}

		~Node()
		{
			//	done in IndexList destructor now
			//delete m_pNext;
		}

	public:
		u32		m_index;
		Node	*m_pNext;
	};

public:
	IndexList():
		m_pRootNode(NULL),
		m_pLastNode(NULL),
		m_indexCount(0)
	{
	}

	~IndexList()
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

	void Insert(u32 index, bool excludeDupes)
	{
		if( m_pRootNode == NULL )
		{
			m_pRootNode = new Node();
			m_pLastNode = m_pRootNode;
			m_pRootNode->m_index = index;
			m_indexCount++;
		}
		else
		{
			bool	found = false;

			if( excludeDupes )
			{
				Node	*currentNode = m_pRootNode;
				while(	currentNode && 
						!found)
				{
					//	check to see if the vert is a dupe
					if( currentNode->m_index == index )
					{
						found = true;
					}

					currentNode = currentNode->m_pNext;
				}
			}

			if( !found )
			{
				m_pLastNode->m_pNext = new Node();
				m_pLastNode = m_pLastNode->m_pNext;
				m_pLastNode->m_index = index;
				m_indexCount++;
			}
		}
	}

public:
	Node	*m_pRootNode;
	Node	*m_pLastNode;
	u32		m_indexCount;
};