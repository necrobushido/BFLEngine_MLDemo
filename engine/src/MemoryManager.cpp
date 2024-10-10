#include "MemoryManager.h"

// Defining this constant enables simple memory link validation every time a memory link is
// accessed.  Has a marginal speed penalty.
//#define MEMVALIDATE

// Defining this constant will intentionally write garbage into all new allocations, to insure
// that no memory consumer is relying on the state of uninitialized memory.  It will also
// write garbage over all freed memory, to insure that no memory consumer relies on the
// persistant state of released memory.  This makes stuff *very* slow :)
//#define WRITE_GARBAGE

// Keeps track of the historical smallest amount of free heap and the historical
// smallest size of the largest free block.  This adds significant time complexity
// to memory allocations, so this should not be left enabled in a final release build.
//#define TRACK_MIN_FREE_HEAP

// Makes the memory manager safe to call from within IRQ.
#define IRQ_SAFE

// Validation macros
#ifdef MEMVALIDATE
#define VALIDATELINK(link) ValidateLink(link)
#else
#define VALIDATELINK(link) link
#endif

// Intentional memory trash test.
#ifdef WRITE_GARBAGE
static u8 g_garbageValue = 1;
#define FREE_GARBAGE_U32 (u32(g_garbageValue) | (u32(g_garbageValue) << 16) | 0xff00ff00)
#define USED_GARBAGE_U32 (u32(g_garbageValue) | (u32(g_garbageValue) << 16) | 0xdd00dd00)
#endif

// Re-entrancy check.
#ifdef IRQ_SAFE
static volatile bool g_bInCall = false;
#define _ENTER() \
	m_mutex.Enter()
#define _EXIT() \
	m_mutex.Exit()
#else
#define _ENTER()
#define _EXIT()
#endif

namespace Memory
{
	void Memset32(void *dest, u32 value, size_t size)
	{
		u32	i;
		for(i = 0; i < (size & (~3)); i+=4)
		{
			memcpy(((char*)dest) + i, &value, 4);
		}  
		for(; i < size; i++)
		{
			((char*)dest)[i] = ((char*)&value)[i&3];
		}  
	}
}

enum
{
	//	these need to be powers of 2 due to bitwise operations
	//	really, they should be anyway
	kAlignment		= 0x04,	//	4 bytes
	kCacheLineSize	= 0x40	//	x86 = 64 bytes?	
};


//----------------------------------------------------------------------------
// MemoryManager
//--

void MemoryManager::Init(void *pPointer, size_t size)
{
	// Point to the first node (which must be 4 byte aligned).  Adjust the heap size
	// to reflect the alignment if necessary
	MemLink	*pLink = reinterpret_cast<MemLink*>((u64(pPointer) + (kAlignment-1)) & ~(kAlignment-1));
	size -= u64(pLink) - u64(pPointer);
	size &= MEM_LINK_TAIL_MASK;

	// Setup the node
	pLink->Set(size, false, true);
	pLink->SetPrev(NULL);
	#ifdef MEMDEBUG
		pLink->SetFileLine(__FILE__, __LINE__);
		pLink->SetDescription("");
	#endif

	// Setup the memory list
	m_pHead = pLink;
	m_pTail = pLink;
	m_pFreeListHead = pLink;
	pLink->SetNextFree(NULL);

	// Store pointers for the top and bottom of memory (for validating the memory links)
	m_memBottom = pLink;
	m_memTop = reinterpret_cast<char*>(pLink) + size;

	// Initialize our historical minumum heap tracking
	#ifdef TRACK_MIN_FREE_HEAP
		m_minFreeHeap = m_min_largestFreeBlock = size;
	#else
		m_minFreeHeap = m_min_largestFreeBlock = 0;
	#endif

	// Initialize the memory history
	#ifdef MEMDEBUG
		for(unsigned i = 0; i < kMemHistorySize; ++i) 
		{
			m_memHistoryFile[i] = "";
			m_memHistoryLine[i] = 0;
			m_memHistorySize[i] = 0;
			m_memHistoryAllocFlag[i] = false;
		}
		m_memHistoryIndex = 0;
	#endif

	// Set the memory list watch flag
	m_watchMemList = false;
}

#ifdef MEMDEBUG
void *MemoryManager::Alloc(const char *file, unsigned line, size_t size)
#else
void *MemoryManager::Alloc(size_t size)
#endif
{
	// IRQ safety
	_ENTER();

	// Add this operation to the memory history
	#ifdef MEMDEBUG
		m_memHistoryFile[m_memHistoryIndex] = file;
		m_memHistoryLine[m_memHistoryIndex] = line;
		m_memHistorySize[m_memHistoryIndex] = size;
		m_memHistoryAllocFlag[m_memHistoryIndex] = true;
		if( ++m_memHistoryIndex >= kMemHistorySize )
		{
			m_memHistoryIndex = 0;
		}
	#endif

	// Check the memory list integrity if we're supposed to be watching it
	if( m_watchMemList )
	{
		WalkMemList();
	}

	// Pad the size to 4 bytes.  Make sure we have at least 4 bytes, becuase we need
	// it to store the freelist link.  Also, add in the size of the MemLink node.
	size_t actualSize = ((size + 0x03) & ~0x03);
	if( actualSize < 4 ) 
	{
		actualSize = 4;
	}
	actualSize = ((actualSize + (kCacheLineSize-1)) & ~(kCacheLineSize-1)); // Force cache line boundary
	actualSize += sizeof(MemLink);

	// Search for the first or last fit (depending on whether we're allocating high or low).
	// Give an error message if we're out of memory
	MemLink		*pLink;
	MemLink		*pPrevFreeLink;
	unsigned	count = 0;
	for(pPrevFreeLink = NULL, pLink = VALIDATELINK(m_pFreeListHead); (pLink != NULL) && (pLink->GetSize() < actualSize); pPrevFreeLink = pLink, pLink = VALIDATELINK(pLink->GetNextFree())) 
	{ 
		count++;
	}

	// Out of memory.
	if( pLink == NULL ) 
	{
		#ifdef MEMDEBUG
			DumpHeap();
			_Panicf("Out of memory trying to allocate %d bytes in module %s, line %d\n", size, file, line);
		#else
			_Panicf("Out of memory trying to allocate %d bytes (use MEMDEBUG for module/line info)\n", size);
		#endif
	}

	// Next and previous links.
	MemLink	*pPrevLink = pLink->GetPrev();
	MemLink	*pNextFreeLink = VALIDATELINK(pLink->GetNextFree());

	// If there isn't enough room to split the block into separate used/free blocks, then
	// just convert the block to used altogether
	MemLink *pNewUsedLink;
	if( pLink->GetSize() <= (actualSize + sizeof(MemLink) + 4) ) 
	{
		if( pPrevFreeLink == NULL )
		{
			m_pFreeListHead = pNextFreeLink;
		}
		else 
		{
			pPrevFreeLink->SetNextFree(pNextFreeLink);
		}
		pLink->MakeUsed();
		pNewUsedLink = pLink;
	}
	else 
	{
		// The block is large enough to split.  Create two blocks
		MemLink	*pHighLink;
		MemLink	*pNextLink = VALIDATELINK(pLink->GetNext());
		size_t	freeSize = (pLink->GetSize() - actualSize);

		pHighLink = reinterpret_cast<MemLink*>(reinterpret_cast<char*>(pLink) + actualSize);
		pHighLink->Set(freeSize, false, (pNextLink == NULL));
		pHighLink->SetNextFree(pNextFreeLink);
		#ifdef MEMDEBUG
			pHighLink->SetFileLine(pLink->GetFile(), pLink->GetLine());
			pHighLink->SetDescription("");
		#endif
		pLink->Set(actualSize, true, false);
		pNewUsedLink = pLink;

		if( pPrevFreeLink == NULL ) 
		{
			m_pFreeListHead = pHighLink;
		}
		else 
		{
			pPrevFreeLink->SetNextFree(pHighLink);
		}

		pHighLink->SetPrev(pLink);
		if( pNextLink != NULL ) 
		{
			pNextLink->SetPrev(pHighLink);
		}
		else 
		{
			m_pTail = pHighLink;
		}
	}

	// Set the module and line number so the source of the allocation can be identified
	#ifdef MEMDEBUG
		pNewUsedLink->SetFileLine(file, line);
		pNewUsedLink->SetDescription("");
	#endif

	// Initialize with garbage if enabled.
	#ifdef WRITE_GARBAGE
		Memory::Memset32((pNewUsedLink + 1), USED_GARBAGE_U32, (actualSize - sizeof(MemLink)));
		if( ++g_garbageValue >= 0x80 ) 
		{
			g_garbageValue = 1;
		}
	#endif

	// Track historical minimum free heap
	#ifdef TRACK_MIN_FREE_HEAP
		// Determine the current free heap size, and the largest free block size.
		size_t	freeMem = 0;
		size_t	largestFreeBlock = 0;
		for(MemLink *pLink = VALIDATELINK(m_pHead); pLink != NULL; pLink = VALIDATELINK(pLink->GetNext())) 
		{
			if( pLink->IsFree() ) 
			{
				if( largestFreeBlock < pLink->GetSize() ) 
				{
					largestFreeBlock = pLink->GetSize();
				}
				freeMem += pLink->GetSize();
			}
		}
		
		// Keep track of the lowest ones we've ever seen.
		if( m_minFreeHeap > freeMem ) 
		{
			m_minFreeHeap = freeMem;
		}
		if( m_min_largestFreeBlock > largestFreeBlock ) 
		{
			m_min_largestFreeBlock = largestFreeBlock;
		}
	#endif

	// IRQ safety
	_EXIT();

	// Return a pointer to the writable portion of the new memory block
	return reinterpret_cast<void*>(pNewUsedLink + 1);
}


#ifdef MEMDEBUG
void MemoryManager::Free(const char *file, unsigned line, void *ptr)
#else
void MemoryManager::Free(void *ptr)
#endif
{
	// IRQ safety
	_ENTER();

	// Add this operation to the memory history
	#ifdef MEMDEBUG
		m_memHistoryFile[m_memHistoryIndex] = file;
		m_memHistoryLine[m_memHistoryIndex] = line;
		m_memHistorySize[m_memHistoryIndex] = 0;
		m_memHistoryAllocFlag[m_memHistoryIndex] = false;
		if( ++m_memHistoryIndex >= kMemHistorySize ) 
		{
			m_memHistoryIndex = 0;
		}
	#endif

	// Check the memory list integrity if we're supposed to be watching it
	if( m_watchMemList ) 
	{
		WalkMemList();
	}

	// Make sure the pointer is valid
	MemLink	*pLink = reinterpret_cast<MemLink *>(ptr) - 1;

	#ifdef MEMDEBUG
		if( ptr == NULL ) 
		{
			_Panicf("Attempt to free NULL pointer in %s, line %d\n", file, line);
		}
		if( (!IsLinkValid(pLink)) ) 
		{
			_Panicf("Attempt to free invalid pointer (%p) in %s, line %d\n", ptr, file, line);
		}
		if( pLink->IsFree() )
		{
			_Panicf("Attempt to free already free pointer (%p) in %s, line %d\n", ptr, file, line);
		}
	#else
		if( ptr == NULL ) 
		{
			_Panic("Attempt to free NULL pointer (use MEMDEBUG for module/line info)\n");
		}
		if( (!IsLinkValid(pLink)) || pLink->IsFree() ) 
		{
			_Panicf("Attempt to free invalid pointer (%p) (use MEMDEBUG for module/line info)\n", ptr);
		}
	#endif

	#ifdef MEMVALIDATE
		MemLink	*pSearchLink;
		for(pSearchLink = VALIDATELINK(m_pHead); (pSearchLink != NULL) && (pSearchLink != pLink); pSearchLink = VALIDATELINK(pSearchLink->GetNext())) { }
		if( pSearchLink == NULL ) 
		{
			_Panic("Attempt to free pointer that is not in the memory list at all\n");
		}
	#endif

	// Make the node free
	pLink->MakeFree();
	#ifdef MEMDEBUG
		pLink->SetFileLine(file, line);
		pLink->SetDescription("");
	#endif
	
	// Rewrite with garbage if enabled.
	#ifdef WRITE_GARBAGE
		Memory::Memset32(reinterpret_cast<char *>(pLink + 1) + sizeof(u32), FREE_GARBAGE_U32, pLink->GetSize() - sizeof(MemLink) - sizeof(u32));
	#endif

	// Point to the surrounding links
	MemLink	*pPrevLink = VALIDATELINK(pLink->GetPrev());
	MemLink *pNextLink = VALIDATELINK(pLink->GetNext());

	// Find our position in the free list
	unsigned	count = 0;
	MemLink		*pPrevFreeLink;
	MemLink		*pNextFreeLink;
	for(pPrevFreeLink = NULL, pNextFreeLink = VALIDATELINK(m_pFreeListHead); (pNextFreeLink != NULL) && (pNextFreeLink < pLink); pPrevFreeLink = pNextFreeLink, pNextFreeLink = VALIDATELINK(pNextFreeLink->GetNextFree())) 
	{ 
		count++;
	}

	// Link oursleves into the free list
	if( pPrevFreeLink == NULL )
	{
		m_pFreeListHead = pLink;
	}
	else 
	{
		pPrevFreeLink->SetNextFree(pLink);
	}
	pLink->SetNextFree(pNextFreeLink);

	// If either of the surrounding links are free also, then merge with them
	if( (pNextLink != NULL) && 
		pNextLink->IsFree() ) 
	{
		pLink->Set((pLink->GetSize() + pNextLink->GetSize()), false, pNextLink->IsLast());
		pLink->SetNextFree(pNextFreeLink->GetNextFree());
		#ifdef WRITE_GARBAGE
			Memory::Memset32(reinterpret_cast<char*>(pNextLink), FREE_GARBAGE_U32, sizeof(MemLink) + sizeof(u32));
		#endif

		if( (pNextLink = VALIDATELINK(pLink->GetNext())) != NULL ) 
		{
			pNextLink->SetPrev(pLink);
		}
		else 
		{
			m_pTail = pLink;
		}
	}

	if( (pPrevLink != NULL) && 
		pPrevLink->IsFree() ) 
	{
		pPrevLink->Set((pPrevLink->GetSize() + pLink->GetSize()), false, pLink->IsLast());
		pPrevFreeLink->SetNextFree(pLink->GetNextFree());
		#ifdef WRITE_GARBAGE
			Memory::Memset32(reinterpret_cast<char*>(pLink), FREE_GARBAGE_U32, sizeof(MemLink) + sizeof(u32));
		#endif

		if( pNextLink != NULL ) 
		{
			pNextLink->SetPrev(pPrevLink);
		}
		else 
		{
			m_pTail = pPrevLink;
		}
	}

	// Advance the garbage value.
	#ifdef WRITE_GARBAGE
		if( ++g_garbageValue >= 0x80 ) 
		{
			g_garbageValue = 1;
		}
	#endif

	// IRQ safety
	_EXIT();
}


void MemoryManager::SetDescription(void *ptr, const char *description)
{
	// We can only do this in MEMDEBUG
	#ifndef MEMDEBUG
		return;
	#endif

	// IRQ safety
	_ENTER();

	// Make sure the pointer is valid
	if( ptr == NULL ) 
	{
		_Panic("Attempt to set description on NULL pointer");
	}
	MemLink	*pLink = reinterpret_cast<MemLink *>(ptr) - 1;
	if( (!IsLinkValid(pLink)) || pLink->IsFree() ) 
	{
		_Panic("Attempt to set description on invalid pointer");
	}

	#ifdef MEMVALIDATE
		MemLink	*pSearchLink;
		for(pSearchLink = VALIDATELINK(m_pHead); (pSearchLink != NULL) && (pSearchLink != pLink); pSearchLink = VALIDATELINK(pSearchLink->GetNext())) { }
		if( pSearchLink == NULL ) 
		{
			_Panic("Attempt to set description on a pointer that is not in the memory list at all");
		}
	#endif

#ifdef MEMDEBUG
	// Set the description
	pLink->SetDescription(description);
#endif

	// IRQ safety
	_EXIT();
}


void MemoryManager::GetState(State *pState)
{
	_ENTER();
	BuildMemState(pState);
	_EXIT();
}


void MemoryManager::DumpMemHistory() const
{
	#ifdef MEMDEBUG
		DebugPrintf("Dumping last %d memory operations:\n", kMemHistorySize);

		for(unsigned i = 0, t; i < kMemHistorySize; ++i) 
		{
			t = ((m_memHistoryIndex + i) % kMemHistorySize);
			if( m_memHistoryFile[t][0] != 0 ) 
			{
				if( m_memHistoryAllocFlag[t] ) 
				{
					DebugPrintf("Allocation: file %s line %d size %d\n", m_memHistoryFile[t], m_memHistoryLine[t], m_memHistorySize[t]);
				}
				else 
				{
					DebugPrintf("Deallocation: file %s line %d\n", m_memHistoryFile[t], m_memHistoryLine[t]);
				}
			}
		}
	#endif
}


void MemoryManager::DumpHeap() const
{
	// Use the current memory state as the header for the dump.
	State	state;
	BuildMemState(&state);

	// The header for the dump.
	DebugPrintf("\nAddress    Size  Type File(line#)                    [description]\n----------------------------------------------------------------------\n");

	// Print a description for every memory link.
	size_t		i;
	size_t		len;
	char		workString[256];
	for(const MemLink *pLink = ValidateLink(GetHead()); pLink != NULL; pLink = ValidateLink(pLink->GetNext())) 
	{
		// Get the filename (module) which owns the memlink.  Skip the path.
		const char	*pFile = pLink->GetFile();
		len = strlen(pFile);
		for(i = len; (i > 0) && (pFile[i-1] != '\\') && (pFile[i-1] != '/'); --i) { }
		pFile = &pFile[i];

		// Output the link description.
		if( pLink->IsUsed() ) 
		{
			sprintf(workString, "%p % 7d Used %s(%u)", pLink, (int)pLink->GetSize(), pFile, pLink->GetLine());
			if( pLink->GetDescription()[0] != 0 ) 
			{
				for(size_t i = strlen(workString); i < 52; ++i) 
				{
					strcat(workString," ");
				}
				sprintf(&workString[strlen(workString)], " [%s]", pLink->GetDescription());
			}
			strcat(workString,"\n");
			DebugPrintf(workString);
		}
		else 
		{
			DebugPrintf("%08x % 7u ----\n", pLink, pLink->GetSize());
		}
	}
	DebugPrintf("----------------------------------------------------------------------\n");

	// General memory stats.
	DebugPrintf("freeMem:%uk usedMem:%uk lrgstFree:%uk numFreeBlks:%u numAllocs:%u\n", state.freeMem >> 10, state.usedMem >> 10, state.largestFreeBlock >> 10, state.numFreeBlocks, state.numAllocations);
	#ifdef TRACK_MIN_FREE_HEAP
		DebugPrintf("historicalMinFreeHeap:%uk historicalMin_largestFreeBlock:%uk\n", state.historicalMinFreeHeap >> 10, state.historicalMin_largestFreeBlock >> 10);
	#endif
}


void MemoryManager::BuildMemState(State *pState) const
{
	// Add up all the memory
	pState->freeMem = 0;
	pState->usedMem = 0;
	pState->largestFreeBlock = 0;
	pState->numFreeBlocks = 0;
	pState->numAllocations = 0;
	for(MemLink *pLink = VALIDATELINK(m_pHead); pLink != NULL; pLink = VALIDATELINK(pLink->GetNext())) 
	{
		if( pLink->IsFree() ) 
		{
			if( pLink->GetSize() > pState->largestFreeBlock ) 
			{
				pState->largestFreeBlock = pLink->GetSize();
			}
			pState->freeMem += pLink->GetSize();
			pState->numFreeBlocks++;
		}
		else 
		{
			pState->usedMem += pLink->GetSize();
			pState->numAllocations++;
		}
	}

	// Report the historical minimum free heap stats.
	pState->historicalMinFreeHeap = m_minFreeHeap;
	pState->historicalMin_largestFreeBlock = m_min_largestFreeBlock;
}


void MemoryManager::WalkMemList() const
{
	// Walk the list and find out if there are any corruptions
	const MemLink	*pLink;
	const MemLink	*pPrevLink;
	size_t			listSize;
	for(pLink = m_pHead, pPrevLink = NULL, listSize = 0; pLink != NULL; listSize += pLink->GetSize(), pPrevLink = pLink, pLink = pLink->GetNext()) 
	{
		if( !IsLinkValid(pLink) ) 
		{
			if( pLink == m_pHead ) 
			{
				DumpMemHistory();
				_Panic("Head of memory list is invalid");
			}
			else 
			{
				DumpMemHistory();
				_Panicf("Last good link: %d bytes at %p from module %s, line %d", pPrevLink->GetSize(), pPrevLink, pPrevLink->GetFile(), pPrevLink->GetLine());
			}
		}
		if( pPrevLink != pLink->GetPrev() ) 
		{
			DumpMemHistory();
			_Panicf("Last good link: %d bytes at %p from module %s, line %d", pPrevLink->GetSize(), pPrevLink, pPrevLink->GetFile(), pPrevLink->GetLine());
		}
	}

	// Walk the free list also
	for(pLink = m_pFreeListHead, pPrevLink = NULL; pLink != NULL; pPrevLink = pLink, pLink = pLink->GetNextFree()) 
	{
		if( !IsLinkValid(pLink) ) 
		{
			if( pLink == m_pFreeListHead ) 
			{
				DumpMemHistory();
				_Panic("Head of free list is invalid");
			}
			else 
			{
				DumpMemHistory();
				_Panicf("Last good free list link: %d bytes at %p from module %s, line %d", pPrevLink->GetSize(), pPrevLink, pPrevLink->GetFile(), pPrevLink->GetLine());
			}
		}
	}

	if( listSize != GetHeapSize() ) 
	{
		DumpMemHistory();
		_Panicf("Memory list and free list look ok, but all nodes in memory list add up to %d bytes (should be %d bytes)", listSize, GetHeapSize());
	}
}


void MemoryManager::MemListCorrupt() const
{
	// Tell the user that the heap is corrupt
	DebugPrintf("MemoryManager Heap Corrupt!!!\n");

	// Walk the memory list to find the problem
	WalkMemList();

	// If we get here, then the list was ok... go figure.
	DumpMemHistory();
	_Panic("List walk reveals no bad links, and memory list size checks out... go figure");
}
