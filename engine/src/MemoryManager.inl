inline const MemLink *MemLink::GetNext() const
{
	return IsLast() ? 0 : reinterpret_cast<const MemLink*>(m_next & MEM_LINK_TAIL_MASK);
}


inline MemLink *MemLink::GetNext()
{
	return IsLast() ? 0 : reinterpret_cast<MemLink*>(m_next & MEM_LINK_TAIL_MASK);
}


inline const MemLink *MemLink::GetPrev() const
{
	return m_pPrev;
}


inline MemLink *MemLink::GetPrev()
{
	return m_pPrev;
}


inline const MemLink *MemLink::GetNextFree() const
{
	return *(reinterpret_cast<const MemLink * const *>(this + 1));
}


inline MemLink *MemLink::GetNextFree()
{
	return *(reinterpret_cast<MemLink**>(this + 1));
}


inline void  MemLink::SetNextFree(const MemLink *pNextFree)
{
	*(reinterpret_cast<const MemLink**>(this + 1)) = pNextFree;
}


inline size_t MemLink::GetSize() const
{
	return (m_next & MEM_LINK_TAIL_MASK) - u64(this);
}


inline bool MemLink::IsUsed() const
{
	return (m_next & 0x01) != 0;
}


inline bool MemLink::IsFree() const
{
	return (m_next & 0x01) == 0;
}


inline bool MemLink::IsLast() const
{
	return (m_next & 0x02) != 0;
}


inline void MemLink::Set(size_t size, bool bUsed, bool bLast)
{
	m_pNext = reinterpret_cast<MemLink *>((u64(this) + size) | (bUsed ? 0x01 : 0) | (bLast ? 0x02 : 0));
}


inline void MemLink::SetPrev(MemLink *pPrev)
{
	m_pPrev = pPrev;
}


#ifdef MEMDEBUG

inline void MemLink::SetFileLine(const char *file, unsigned line)
{
	m_pFile = file;
	m_line = line;
}


inline void MemLink::SetDescription(const char *description)
{
	m_pDescription = description;
}

#endif


inline void MemLink::MakeUsed()
{
	m_next |= 0x01;
}


inline void MemLink::MakeFree()
{
	m_next &= ~0x01;
}


#ifdef MEMDEBUG

inline const char *MemLink::GetFile() const
{
	return m_pFile;
}


inline unsigned MemLink::GetLine() const
{
	return m_line;
}

inline const char *MemLink::GetDescription() const
{
	return m_pDescription;
}

#else

inline const char *MemLink::GetFile() const
{
	return "<Use MEMDEBUG>";
}


inline unsigned MemLink::GetLine() const
{
	return 0;
}

inline const char *MemLink::GetDescription() const
{
	return "";
}

#endif


//----------------------------------------------------------------------------
// MemoryManager
//--

#ifdef MEMDEBUG

inline void *MemoryManager::Alloc(size_t size)
{
	return Alloc("<Not Given>", 0, size);
}


inline void MemoryManager::Free(void *ptr)
{
	Free("<Not Given>", 0, ptr);
}

#else

inline void *MemoryManager::Alloc(const char *file, unsigned line, size_t size)
{
	return Alloc(size);
}


inline void MemoryManager::Free(const char *file, unsigned line, void *ptr)
{
	Free(ptr);
}

#endif


inline size_t MemoryManager::GetHeapSize() const
{
	return size_t(reinterpret_cast<const char *>(m_memTop) - reinterpret_cast<const char *>(m_memBottom));
}


inline MemLink *MemoryManager::GetHead() const
{
	return m_pHead;
}


inline MemLink *MemoryManager::GetTail() const
{
	return m_pTail;
}


inline bool MemoryManager::IsLinkValid(const MemLink *pLink) const
{
	if( ((u64(pLink) & 0x03) == 0) && 
		(pLink >= m_memBottom) && 
		(pLink < m_memTop) ) 
	{
		const void	*pNextPtr = reinterpret_cast<const char *>(pLink) + pLink->GetSize();
		return (pNextPtr > pLink) && (pNextPtr <= m_memTop);
	}
	return false;
}


inline MemLink *MemoryManager::ValidateLink(MemLink *pLink) const
{
	if( (pLink != 0) && 
		(!IsLinkValid(pLink)) )
	{
		MemListCorrupt();
	}
	return pLink;
}


inline const MemLink *MemoryManager::ValidateLink(const MemLink *pLink) const
{
	if( (pLink != 0) && 
		(!IsLinkValid(pLink)) ) 
	{
		MemListCorrupt();
	}
	return pLink;
}


inline void MemoryManager::SetListWatch(bool watchMemList)
{
	m_watchMemList = watchMemList;
}


inline void MemoryManager::WatchMemList() const
{
	if( m_watchMemList ) 
	{
		WalkMemList();
	}
}

inline void MemoryManager::CheckMemList() const
{
	WalkMemList();
}
