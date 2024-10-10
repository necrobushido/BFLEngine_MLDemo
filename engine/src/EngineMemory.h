#pragma once

#include "dataTypes.h"

// Memory state structure
struct MemState 
{
	size_t		freeMem;
	size_t		usedMem;
	size_t		largestFreeBlock;
	unsigned	numFreeBlocks;
	unsigned	numAllocations;
};

namespace Memory 
{
	void Init(void *heapPtr, size_t heapSize);
	void Shutdown();

	// Alloc/free.  Under normal circumstances, do not call these directly,
	// but rather use the MemAlloc and MemFree macros (or new / delete).
	void *Alloc(size_t size);
	void *Alloc(const char *file, unsigned line, size_t size);
	void Free(void *ptr);
	void Free(const char *file, unsigned line, void *ptr);

	// For debugging, a pointer may be described with a persistent string.
	void SetPtrDescription(void *ptr, const char *description);

	// State
	void GetMemState(MemState *pState);
	unsigned GetHeapUsedListCount();
	unsigned GetHeapFreeListCount();

	// Dumps the high-level details of the memory state to the console.
	void Report();

	// Does an exhaustive dump of the memory list.
	void DumpHeap();

	// Does a thorough check of the memory list, making sure that all links are good and
	// that the used & free list add up to the proper amount of memory.
	void CheckHeap();

	// These are named kind of akward.  "Watch" means the same thing as CheckHeap(), but
	// only when the watch flag is enabled.  It is disabled by default.  When watch is enabled,
	// every alloc/free operation will automatically perform a full CheckHeap().  ListWatch() allows
	// this conditional CheckHeap() to be placed in other sections of code as well.
	void SetListWatch(bool bWatchFlag);
	void ListWatch();

	// Helper function to display the current free heap with some description attached.
	void PrintHeapFreeSize(char *description);

	//	
	inline void CpuCopy8(void *srcPtr, void *dstPtr, size_t byteCount)
	{
		memcpy(srcPtr, dstPtr, byteCount);
	}

	inline void CpuFill32(void *srcPtr, u32 data, u32 size)
	{
		for(u32 offset = 0; offset < size; offset += 4)
		{
			*((u32*)&(((u8*)srcPtr)[offset])) = data;
		}
	}
}

//----------------------------------------------------------------------------
// Alloc / free macros
//--
#ifdef MEMDEBUG
#define MemAlloc(size) Memory::Alloc(__FILE__, __LINE__, size)
#define MemFree(ptr) Memory::Free(__FILE__, __LINE__, ptr)
#else
#define MemAlloc(size) Memory::Alloc(size)
#define MemFree(ptr) Memory::Free(ptr)
#endif // MEMDEBUG

//----------------------------------------------------------------------------
// New / delete operators
//--
//	placement new; already defined
//inline void *operator new(size_t size, void *ptr) 
//{ 
//	return ptr; 
//}

//void *operator new(size_t size);
//void *operator new[](size_t size);
//
//void *operator new(size_t size, const char *file, unsigned line);
//void *operator new[](size_t size, const char *file, unsigned line);
//
//#ifdef MEMDEBUG
//extern const char *g_pOperatorDeleteFile;
//extern unsigned g_operatorDeleteLine;
//#define _delete(ptr) { g_pOperatorDeleteFile = __FILE__; g_operatorDeleteLine = __LINE__; delete ptr; }
//#else
//#define _delete(ptr) delete ptr;
//#endif // MEMDEBUG
//
//void	operator delete(void *ptr);
//void	operator delete[](void *ptr);
//void	operator delete(void *ptr, const char *file, unsigned line);
//void	operator delete[](void *ptr, const char *file, unsigned line);
//
//#ifdef MEMDEBUG
//#define NEWMACRO
//#include "defnew.h"
//#endif // MEMDEBUG
