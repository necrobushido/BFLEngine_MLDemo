#pragma once

#include "types.h"
#include "ListMacros.h"

template<class T> class HashTable
{
public:
	struct TableEntry 
	{
		LIST_DECLARE(HashSiblings, T);
	};

	struct EntryMember
	{
		LIST_LINK(HashSiblings, T);
	};

public:
	HashTable(int hashBits);
	~HashTable();

public:
	T *GetEntry(int hash);
	void AddEntry(T *entry, int hash);
	void RemoveEntry(T *entry, int hash);

public:
	int			m_numHashBits;
	TableEntry	*m_hashTable;
};

#include "HashTable.inl"