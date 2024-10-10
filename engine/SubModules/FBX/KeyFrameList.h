#pragma once

#include "stdafx.h"

class KeyFrameList
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
			//delete m_pNext;
		}

	public:
		f32			m_time;
		Node*		m_pNext;
	};

public:
	KeyFrameList():
		m_pRootNode(NULL),
		m_keyframeCount(0)
	{
	}

	~KeyFrameList()
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
	void Insert(f32 time)
	{
		if( m_pRootNode == NULL )
		{
			m_pRootNode = new Node();
			m_pRootNode->m_time = time;
			m_keyframeCount++;
		}
		else
		{
			bool	found = false;
			Node*	currentNode = m_pRootNode;
			Node*	nodeToUseAsPrev = NULL;
			while(currentNode && !found)
			{
				//	check to see if it's a dupe
				if( currentNode->m_time == time )
				{
					//	if it is, don't insert
					found = true;
				}
				else
				if( currentNode->m_time < time )
				{
					nodeToUseAsPrev = currentNode;
				}
				else
				//	currentNode->m_time > time
				{
				}

				currentNode = currentNode->m_pNext;
			}

			//	not a dupe?, insert it into the list, in sorted order
			if( !found )
			{
				Node*	newNode = new Node();
				if( nodeToUseAsPrev == NULL )
				{
					//	new root node
					newNode->m_pNext = m_pRootNode;
					m_pRootNode = newNode;
				}
				else
				{
					//	insert in the appropriate spot					
					newNode->m_pNext = nodeToUseAsPrev->m_pNext;
					nodeToUseAsPrev->m_pNext = newNode;
				}
				newNode->m_time = time;
				m_keyframeCount++;
			}
		}
	}

public:
	Node*	m_pRootNode;
	u32		m_keyframeCount;
};