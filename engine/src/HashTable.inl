template<class T>
inline HashTable<T>::HashTable(int hashBits):
	m_numHashBits(hashBits)
{
	m_hashTable = new TableEntry[(u64)1 << m_numHashBits];
}

template<class T>
inline HashTable<T>::~HashTable()
{
	delete [] m_hashTable;
}

template<class T>
inline T *HashTable<T>::GetEntry(int hash)
{
	T				*pEntry;
	const unsigned	hashTableIndex = hash & ((1 << m_numHashBits) - 1);
	pEntry = LIST_HEAD(&m_hashTable[hashTableIndex], HashSiblings);
	while(	(pEntry != NULL) && 
			(pEntry->hash != hash) )
	{
		pEntry = LIST_NEXT(pEntry, HashSiblings);
	}
	return pEntry;
}

template<class T>
inline void HashTable<T>::AddEntry(T *entry, int hash)
{
	const unsigned	hashTableIndex = hash & ((1 << m_numHashBits) - 1);
	LIST_ADD(&m_hashTable[hashTableIndex], HashSiblings, entry);
}

template<class T>
inline void HashTable<T>::RemoveEntry(T *entry, int hash)
{
	const unsigned	hashTableIndex = hash & ((1 << m_numHashBits) - 1);
	LIST_REMOVE(&m_hashTable[hashTableIndex], HashSiblings, entry);
}