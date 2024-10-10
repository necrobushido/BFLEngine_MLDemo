#pragma once

#include "types.h"

#include "Mutex.h"

#define MEM_LINK_TAIL_MASK	0xfffffffffffffffc

class MemLink 
{
public:
	// Node list (used & free nodes)
	const MemLink *GetNext() const;
	MemLink *GetNext();
	const MemLink *GetPrev() const;
	MemLink *GetPrev();

	// Free list (free nodes only)
	const MemLink *GetNextFree() const;
	MemLink *GetNextFree();
	void SetNextFree(const MemLink *pNextFree);

	// Size
	size_t GetSize() const;

	bool IsUsed() const;
	bool IsFree() const;

	bool IsLast() const;

	void Set(size_t size, bool bUsed, bool bLast);

	void SetPrev(MemLink *pPrev);

	#ifdef MEMDEBUG
		void SetFileLine(const char *file, unsigned line);
		void SetDescription(const char *description);
	#endif

	void MakeUsed();
	void MakeFree();

	const char *GetFile() const;
	unsigned GetLine() const;
	const char *GetDescription() const;

private:
	union 
	{
		MemLink*	m_pNext;
		uintptr_t	m_next;
	};

	MemLink*	m_pPrev;

	#ifdef MEMDEBUG
		const char	*m_pFile;
		u32			m_line;
		const char	*m_pDescription;
	#else
		u32			pad1, pad2, pad3;
	#endif
	u32				pad[3];
};

class MemoryManager 
{
public:
	// Initialization
	void Init(void *heap, size_t heapSize);

	// Alloc / free
	#ifdef MEMDEBUG
		void *Alloc(size_t size);
		void *Alloc(const char *file, unsigned line, size_t size);
		void Free(void *ptr);
		void Free(const char *file, unsigned line, void *ptr);
	#else
		void *Alloc(size_t size);
		void *Alloc(const char *file, unsigned line, size_t size);
		void Free(void *ptr);
		void Free(const char *file, unsigned line, void *ptr);
	#endif
	
	// Add a description to a pointer (MEMDEBUG only)
	void SetDescription(void *ptr, const char *description);

	// Memory state
	struct State 
	{
		size_t		freeMem;
		size_t		usedMem;
		size_t		largestFreeBlock;
		unsigned	numFreeBlocks;
		unsigned	numAllocations;

		// Will be zero if these features are not enabled.
		size_t		historicalMinFreeHeap;
		size_t		historicalMin_largestFreeBlock;
	};

	void GetState(State *pState);
	size_t GetHeapSize() const;

	// Memory list functions
	MemLink *GetHead() const;
	MemLink *GetTail() const;
	bool IsLinkValid(const MemLink *pLink) const;
	MemLink *ValidateLink(MemLink *pLink) const;
	const MemLink *ValidateLink(const MemLink *pLink) const;

	// Debug
	void DumpMemHistory() const;
	void SetListWatch(bool watchMemList);
	void WatchMemList() const;
	void CheckMemList() const;
	void DumpHeap() const;

private:
	// Builds a memory state structure for debugging
	void BuildMemState(State *pState) const;

	void WalkMemList() const;
	void MemListCorrupt() const;

private:
	// Node list (used & free nodes)
	MemLink	*m_pHead;
	MemLink *m_pTail;

	// Free list
	MemLink *m_pFreeListHead;	

	// Used for link validation (if it is turned on).
	const void	*m_memBottom;
	const void	*m_memTop;
	
	// Historical minimum amount of free heap
	size_t	m_minFreeHeap;
	size_t	m_min_largestFreeBlock;

	// History of the last 100 memory operations.
	enum { kMemHistorySize = 100 };
	#ifdef MEMDEBUG
		const char		*m_memHistoryFile[kMemHistorySize];
		unsigned		m_memHistoryLine[kMemHistorySize];
		size_t			m_memHistorySize[kMemHistorySize];
		unsigned char	m_memHistoryAllocFlag[kMemHistorySize];
		unsigned		m_memHistoryIndex;
	#endif

	bool	m_watchMemList;

	Mutex	m_mutex;
};

#include "MemoryManager.inl"