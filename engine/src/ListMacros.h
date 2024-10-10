//----------------------------------------------------------------------------
//--
//-- ListMacros.h - List Macros
//--
//-- I know, these are horribly ugly.  But, they allow me to place objects on
//-- multiple lists without allocating separate link objects, or inheriting from
//-- multiple link base classes.
//--

#ifndef LISTMACROS_H
#define LISTMACROS_H
#pragma once

//----------------------------------------------------------------------------
// List declarations for both doubly and singly linked lists
//--

#define LIST_DECLARE(listName, typeName) \
	struct listName##_ControlStruct { \
		listName##_ControlStruct() : pHead(NULL), pTail(NULL) { } \
		typeName *pHead, *pTail; \
	} control_##listName

#define LIST_DECLARE_NOCONSTRUCT(listName, typeName) \
	struct listName##_ControlStruct { \
		typeName *pHead, *pTail; \
	} control_##listName

#define LIST_INIT(pLister,listName) (pLister)->control_##listName.pHead = (pLister)->control_##listName.pTail = NULL

#define LIST_HEAD(pLister,listName) (pLister)->control_##listName.pHead
#define LIST_TAIL(pLister,listName) (pLister)->control_##listName.pTail

#define LIST_EMPTY(pLister,listName) ( ! LIST_HEAD(pLister,listName) )

//----------------------------------------------------------------------------
// Double linked lists
//--

#define LIST_ADD(pLister, listName, pMember) \
	if((pLister)->control_##listName.pTail == NULL) { \
		(pLister)->control_##listName.pTail = (pLister)->control_##listName.pHead = (pMember); \
		(pMember)->link_##listName.pNext = (pMember)->link_##listName.pPrev = NULL; \
	} \
	else { \
		(pLister)->control_##listName.pTail->link_##listName.pNext = (pMember); \
		(pMember)->link_##listName.pPrev = (pLister)->control_##listName.pTail; \
		(pLister)->control_##listName.pTail = (pMember); \
		(pMember)->link_##listName.pNext = NULL; \
	}

#define LIST_ADDFRONT(pLister, listName, pMember) \
	if((pLister)->control_##listName.pHead == NULL) { \
		(pLister)->control_##listName.pHead = (pLister)->control_##listName.pTail = (pMember); \
		(pMember)->link_##listName.pPrev = (pMember)->link_##listName.pNext = NULL; \
	} \
	else { \
		(pLister)->control_##listName.pHead->link_##listName.pPrev = (pMember); \
		(pMember)->link_##listName.pNext = (pLister)->control_##listName.pHead; \
		(pLister)->control_##listName.pHead = (pMember); \
		(pMember)->link_##listName.pPrev = NULL; \
	}

#define LIST_INSERT(pLister, listName, pAt, pMember) \
	if((pAt) == NULL) { \
		LIST_ADD(pLister,listName,pMember); \
	} \
	else { \
		(pMember)->link_##listName.pNext = (pAt); \
		(pMember)->link_##listName.pPrev = (pAt)->link_##listName.pPrev; \
		(pAt)->link_##listName.pPrev = (pMember); \
		if((pMember)->link_##listName.pPrev != NULL) { \
			(pMember)->link_##listName.pPrev->link_##listName.pNext = (pMember); \
		} \
		else { \
			(pLister)->control_##listName.pHead = (pMember); \
		} \
	}

#define LIST_INSERTAFTER(pLister, listName, pAt, pMember) \
	if((pAt) == NULL) { \
		LIST_ADDFRONT(pLister,listName,pMember); \
	} \
	else { \
		(pMember)->link_##listName.pPrev = (pAt); \
		(pMember)->link_##listName.pNext = (pAt)->link_##listName.pNext; \
		(pAt)->link_##listName.pNext = (pMember); \
		if((pMember)->link_##listName.pNext != NULL) { \
			(pMember)->link_##listName.pNext->link_##listName.pPrev = (pMember); \
		} \
		else { \
			(pLister)->control_##listName.pTail = (pMember); \
		} \
	}

#define LIST_REMOVE(pLister, listName, pMember) \
	if((pMember)->link_##listName.pNext != NULL) { \
		(pMember)->link_##listName.pNext->link_##listName.pPrev = (pMember)->link_##listName.pPrev; \
	} \
	else { \
		(pLister)->control_##listName.pTail = (pMember)->link_##listName.pPrev; \
	} \
	if((pMember)->link_##listName.pPrev != NULL) { \
		(pMember)->link_##listName.pPrev->link_##listName.pNext = (pMember)->link_##listName.pNext; \
	} \
	else { \
		(pLister)->control_##listName.pHead = (pMember)->link_##listName.pNext; \
	} \
	(pMember)->link_##listName.pNext = (pMember)->link_##listName.pPrev = NULL;


#define LIST_ADDLIST(pLister, listName, pFromLister) \
	if((pLister)->control_##listName.pTail != NULL) { \
		if((pFromLister)->control_##listName.pHead != NULL) { \
			(pLister)->control_##listName.pTail->link_##listName.pNext = (pFromLister)->control_##listName.pHead; \
			(pFromLister)->control_##listName.pHead->link_##listName.pPrev = (pLister)->control_##listName.pTail; \
			(pLister)->control_##listName.pTail = (pFromLister)->control_##listName.pTail; \
		} \
	} \
	else { \
		(pLister)->control_##listName.pTail = (pFromLister)->control_##listName.pTail; \
		(pLister)->control_##listName.pHead = (pFromLister)->control_##listName.pHead; \
	} \
	(pFromLister)->control_##listName.pHead = (pFromLister)->control_##listName.pTail = NULL

#define LIST_ADDLISTFRONT(pLister, listName, pFromLister) \
	if((pLister)->control_##listName.pHead != NULL) { \
		if((pFromLister)->control_##listName.pTail != NULL) { \
			(pLister)->control_##listName.pHead->link_##listName.pPrev = (pFromLister)->control_##listName.pTail; \
			(pFromLister)->control_##listName.pTail->link_##listName.pNext = (pLister)->control_##listName.pHead; \
			(pLister)->control_##listName.pHead = (pFromLister)->control_##listName.pHead;  \
		} \
	} \
	else { \
		(pLister)->control_##listName.pHead = (pFromLister)->control_##listName.pHead; \
		(pLister)->control_##listName.pTail = (pFromLister)->control_##listName.pTail; \
	} \
	(pFromLister)->control_##listName.pHead = (pFromLister)->control_##listName.pTail = NULL

///////////////////////
// added 11/2 -jeff lee
#define LIST_CLEAR( pLister, listName, typeName, bDelete ) \
	do { \
		typeName* current##listName; \
		typeName* next##listName = LIST_HEAD( pLister, listName ); \
		while ( next##listName ) { \
			current##listName = next##listName; \
			next##listName    = LIST_NEXT( current##listName, listName ); \
			LIST_REMOVE( pLister, listName, current##listName ); \
			if ( bDelete ) delete current##listName; \
		} \
	} while ( false )

#define LIST_CLEAR_DELETE( pLister, listName, typeName )	LIST_CLEAR( pLister, listName, typeName, true )
#define LIST_CLEAR_NODELETE( pLister, listName, typeName )	LIST_CLEAR( pLister, listName, typeName, false )

/////////////////////////
// added 3/6/07 -jeff lee
// Calls a common method on each object in a list.
#define LIST_FOREACH( pLister, listName, typeName, method ) \
	do { \
		typeName* current##listName; \
		typeName* next##listName = LIST_HEAD( pLister, listName ); \
		while ( next##listName ) { \
			current##listName = next##listName; \
			next##listName    = LIST_NEXT( current##listName, listName ); \
			current##listName->method; \
		} \
	} while ( false )

#define LIST_REVERSE(pLister, listName, typeName) \
	{   \
		typeName* cur##listName = (pLister)->control_##listName.pHead;   \
		while( cur##listName ){   \
			typeName* temp##listName = cur##listName->link_##listName.pNext;   \
			cur##listName->link_##listName.pNext = cur##listName->link_##listName.pPrev;   \
			cur##listName->link_##listName.pPrev = temp##listName;   \
			cur##listName = temp##listName;   \
		}   \
		cur##listName = (pLister)->control_##listName.pHead;   \
		(pLister)->control_##listName.pHead = (pLister)->control_##listName.pTail;   \
		(pLister)->control_##listName.pTail = cur##listName;   \
	}
//----------------------------------------------------------------------------
// Single linked lists
//--

#define SLIST_ADD(pLister, listName, pMember) \
	if((pLister)->control_##listName.pTail == NULL) { \
		(pLister)->control_##listName.pTail = (pLister)->control_##listName.pHead = (pMember); \
		(pMember)->link_##listName.pNext = NULL; \
	} \
	else { \
		(pLister)->control_##listName.pTail->link_##listName.pNext = (pMember); \
		(pLister)->control_##listName.pTail = (pMember); \
		(pMember)->link_##listName.pNext = NULL; \
	}

#define SLIST_ADDFRONT(pLister, listName, pMember) \
	if((pLister)->control_##listName.pHead == NULL) { \
		(pLister)->control_##listName.pHead = (pLister)->control_##listName.pTail = (pMember); \
		(pMember)->link_##listName.pNext = NULL; \
	} \
	else { \
		(pMember)->link_##listName.pNext = (pLister)->control_##listName.pHead; \
		(pLister)->control_##listName.pHead = (pMember); \
	}

#define SLIST_POPFRONT(pLister, listName) \
	if((pLister)->control_##listName.pHead != NULL) { \
		if(((pLister)->control_##listName.pHead = (pLister)->control_##listName.pHead->link_##listName.pNext) == NULL) { \
			(pLister)->control_##listName.pTail = NULL; \
		} \
	}

#define SLIST_ADDLIST(pLister, listName, pFromLister) \
	if((pLister)->control_##listName.pTail != NULL) { \
		(pLister)->control_##listName.pTail->link_##listName.pNext = (pFromLister)->control_##listName.pHead; \
	} \
	else { \
		(pLister)->control_##listName.pTail = (pLister)->control_##listName.pHead = (pFromLister)->control_##listName.pHead; \
	} \
	(pFromLister)->control_##listName.pHead = (pFromLister)->control_##listName.pTail = NULL;

#define SLIST_ADDLISTFRONT(pLister, listName, pFromLister) \
	if((pFromLister)->control_##listName.pTail != NULL) { \
		(pFromLister)->control_##listName.pTail->link_##listName.pNext = (pLister)->control_##listName.pHead; \
		(pLister)->control_##listName.pHead = (pFromLister)->control_##listName.pHead; \
		(pFromLister)->control_##listName.pHead = (pFromLister)->control_##listName.pTail = NULL; \
	} \


//----------------------------------------------------------------------------
// Double links
//--

#define LIST_LINK(listName, typeName) \
	struct listName##_LinkStruct { \
		listName##_LinkStruct() : pNext(NULL), pPrev(NULL) { } \
		typeName *pNext, *pPrev; \
	} link_##listName

#define LIST_LINK_NOCONSTRUCT(listName, typeName) \
	struct listName##_LinkStruct { \
		typeName *pNext, *pPrev; \
	} link_##listName

#define LIST_INITLINK(pMember, listName) (pMember)->link_##listName.pNext = (pMember)->link_##listName.pPrev = NULL

#define LIST_NEXT(pMember, listName) (pMember)->link_##listName.pNext
#define LIST_PREV(pMember, listName) (pMember)->link_##listName.pPrev

#define LIST_ISMEMBER(pLister, pMember, listName) (((pMember)->link_##listName.pNext != NULL) || ((pMember)->link_##listName.pPrev != NULL) || ((pLister)->control_##listName.pHead == (pMember)) || ((pLister)->control_##listName.pTail == (pMember)))


//----------------------------------------------------------------------------
// Single links
//--

#define SLIST_LINK(listName, typeName) \
	struct listName##_LinkStruct { \
		listName##_LinkStruct() : pNext(NULL) { } \
		typeName *pNext; \
	} link_##listName

#define SLIST_LINK_NOCONSTRUCT(listName, typeName) \
	struct listName##_LinkStruct { \
		typeName *pNext, *pPrev; \
	} link_##listName

#define SLIST_INITLINK(pMember, listName) (pMember)->link_##listName.pNext = NULL

#define SLIST_NEXT(pMember, listName) (pMember)->link_##listName.pNext

#define SLIST_ISMEMBER(pLister, pMember, listName) (((pMember)->link_##listName.pNext != NULL) || ((pLister)->control_##listName.pHead == (pMember)) || ((pLister)->control_##listName.pTail == (pMember)))


//----------------------------------------------------------------------------
// Hashtable macros
//--

/*
#define HASHTABLE_DECLARE(NAME, TYPE, NUMBINSEXP) \
	struct NAME##_HashTableControl { \
		struct Bin { \
			LIST_DECLARE_NOCONSTRUCT(NAME##_BinList, TYPE); \
		}; \
		Bin m_bins[1 << NUMBINSEXP]; \
		NAME##_HashTableControl() { \
			MI_CpuClearFast(m_bins, sizeof(m_bins)); \
		} \
		TYPE *pHead, *pTail; \
	} NAME##_hshtbl_control

//#define HASHTABLE_DECLARE_NOCONSTRUCT

#define HASHTABLE_LINK(NAME, TYPE) \
	struct NAME##_HashTableLink { \
		LIST_LINK(NAME##_BinList, TYPE); \
	} NAME##_hshtbl_link

// #define HASHTABLE_LINK_NOCONSTRUCT


#define HASHTABLE_ADD(pLister, NAME, pMember) \
	if((pLister)->NAME##_control.pTail == NULL) { \
		(pLister)->NAME##_control.pTail = (pLister)->NAME##_control.pHead = (pMember); \
		(pMember)->NAME##_link.pNext = (pMember)->NAME##_link.pPrev = NULL; \
	} \
	else { \
		(pLister)->NAME##_control.pTail->NAME##_link.pNext = (pMember); \
		(pMember)->NAME##_link.pPrev = (pLister)->NAME##_control.pTail; \
		(pLister)->NAME##_control.pTail = (pMember); \
		(pMember)->NAME##_link.pNext = NULL; \
	}
*/	


#endif // LISTMACROS_H
