//----------------------------------------------------------------------------
//--
//-- Memory.cpp
//--

#include "EngineMemory.h"
#include "MemoryManager.h"

// Don't use the new macros in here.
#include "undefnew.h"

// Delete operator hack.
const char*	g_pOperatorDeleteFile = "<deleteop>";
unsigned	g_operatorDeleteLine = 0;

// instances of MemoryManager
static MemoryManager	g_memmgr;

namespace Memory 
{

void Init(void *heapPtr, size_t heapSize)
{
	// Initialize our fast MemoryManager instance.
	g_memmgr.Init(heapPtr, heapSize);

	// Report the initial memory configuration.
	Report();
}

void Shutdown()
{
}

void *Alloc(size_t size)
{
	void	*ptr = g_memmgr.Alloc(size);
	return ptr;
}

void *Alloc(const char *file, unsigned line, size_t size)
{
	void	*ptr = g_memmgr.Alloc(file, line, size);
	return ptr;
}

void Free(void *ptr)
{
	if( ptr != NULL )
	{
		g_memmgr.Free(ptr);
	}
}

void Free(const char *file, unsigned line, void *ptr)
{
	if( ptr != NULL )
	{
		g_memmgr.Free(file, line, ptr);
	}
}


void SetPtrDescription(void *ptr, const char *description)
{
	g_memmgr.SetDescription(ptr, description);
}

void GetMemState(MemState *pMemState)
{
	// Get the g_memmgr state and copy it into our state structure.  I know it's
	// weird that we do not use the same structure definition, but the intent is
	// the lower-level module that may be swapped out with some other structure
	// which would have it's own way of reporting state.
	MemoryManager::State	state;
	g_memmgr.GetState(&state);
	pMemState->freeMem = state.freeMem;
	pMemState->usedMem = state.usedMem;
	pMemState->largestFreeBlock = state.largestFreeBlock;
	pMemState->numFreeBlocks = state.numFreeBlocks;
	pMemState->numAllocations = state.numAllocations;
}

unsigned GetHeapUsedListCount()
{
	MemoryManager::State	state;
	g_memmgr.GetState(&state);
	return state.numAllocations;
}

unsigned GetHeapFreeListCount()
{
	MemoryManager::State	state;
	g_memmgr.GetState(&state);
	return state.numFreeBlocks;
}

void Report()
{
	// Report
	MemState	memstate;
	GetMemState(&memstate);
	DebugPrintf("freeMem:%uk usedMem:%uk lrgstFree:%uk numFreeBlks:%d numAllocs:%d\n", memstate.freeMem >> 10, memstate.usedMem >> 10, memstate.largestFreeBlock >> 10, memstate.numFreeBlocks, memstate.numAllocations);
}

void DumpHeap()
{
	g_memmgr.DumpHeap();
}


void CheckHeap()
{
	g_memmgr.CheckMemList();
}


void SetListWatch(bool bWatchFlag)
{
	g_memmgr.SetListWatch(bWatchFlag);
}


void ListWatch()
{
	g_memmgr.WatchMemList();
}


void PrintHeapFreeSize(char *description)
{
	MemoryManager::State	state;
	g_memmgr.GetState(&state);
	DebugPrintf("FreeHeap: %u  ", state.freeMem);
	if( state.historicalMinFreeHeap != 0 ) 
	{
		DebugPrintf("historicalMinFreeHeap: %u  historicalMin_largestFreeBlock:%u  ", state.historicalMinFreeHeap, state.historicalMin_largestFreeBlock);
	}
	DebugPrintf("location: %s\n", description);
}


} // namespace Memory

//void *operator new(size_t size)
//{
//	#ifdef MEMDEBUG
//	void *ptr = g_memmgr.Alloc("<newop>", 0, size);
//	#else
//		void *ptr = g_memmgr.Alloc(size);
//	#endif
//	return ptr;
//}
//
//void *operator new[](size_t size)
//{
//	#ifdef MEMDEBUG
//		void *ptr = g_memmgr.Alloc("<newop>", 0, size);
//	#else
//		void *ptr = g_memmgr.Alloc(size);
//	#endif
//	return ptr;
//}
//
//void *operator new(size_t size,const char *file,unsigned line)
//{
//	#ifdef MEMDEBUG
//		void *ptr = g_memmgr.Alloc(file, line, size);
//	#else
//		void *ptr = g_memmgr.Alloc(size);
//	#endif
//	return ptr;
//}
//
//void *operator new[](size_t size,const char *file,unsigned line)
//{
//	#ifdef MEMDEBUG
//		void *ptr = g_memmgr.Alloc(file, line, size);
//	#else
//		void *ptr = g_memmgr.Alloc(size);
//	#endif
//	return ptr;
//}
//
//void operator delete(void *ptr)
//{
//	// C++ standard says delete 0 is 'guaranteed to be harmless'
//	if( ptr == NULL ) 
//	{
//		return;
//	}
//	
//	#ifdef MEMDEBUG
//		g_memmgr.Free(g_pOperatorDeleteFile, g_operatorDeleteLine, ptr);
//		g_pOperatorDeleteFile = "<deleteop>";
//		g_operatorDeleteLine = 0;
//	#else
//		g_memmgr.Free(ptr);
//	#endif
//}
//
//void operator delete[](void *ptr)
//{
//	// C++ standard says delete 0 is 'guaranteed to be harmless'
//	if( ptr == NULL ) 
//	{
//		return;
//	}
//
//	#ifdef MEMDEBUG
//		g_memmgr.Free(g_pOperatorDeleteFile, g_operatorDeleteLine, ptr);
//		g_pOperatorDeleteFile = "<deleteop>";
//		g_operatorDeleteLine = 0;
//	#else
//		g_memmgr.Free(ptr);
//	#endif
//}
//
//void operator delete(void *ptr, const char *file, unsigned line)
//{
//	// C++ standard says delete 0 is 'guaranteed to be harmless'
//	if( ptr == NULL ) 
//	{
//		return;
//	}
//	
//	#ifdef MEMDEBUG
//		g_memmgr.Free(g_pOperatorDeleteFile, g_operatorDeleteLine, ptr);
//		g_pOperatorDeleteFile = "<deleteop>";
//		g_operatorDeleteLine = 0;
//	#else
//		g_memmgr.Free(ptr);
//	#endif
//}
//
//void operator delete[](void *ptr, const char *file, unsigned line)
//{
//	// C++ standard says delete 0 is 'guaranteed to be harmless'
//	if( ptr == NULL ) 
//	{
//		return;
//	}
//	
//	#ifdef MEMDEBUG
//		g_memmgr.Free(g_pOperatorDeleteFile, g_operatorDeleteLine, ptr);
//		g_pOperatorDeleteFile = "<deleteop>";
//		g_operatorDeleteLine = 0;
//	#else
//		g_memmgr.Free(ptr);
//	#endif
//}