#pragma once

#include "dataTypes.h"

template<class T>
class Array
{
public:
	enum
	{
		kAllocInc = 4
	};

public:
	Array():
		m_data(NULL),
		m_count(0),
		m_numAllocated(0)
	{
	}

	Array(const Array &a2):
		m_data(NULL),
		m_count(0),
		m_numAllocated(0)
	{
		Alloc(a2.m_numAllocated);
		m_count = a2.m_count;

		for(u32 i = 0; i < m_count; ++i)
		{
#include "undefnew.h"
			(void) new (&m_data[i]) T;
#include "defnew.h"
			m_data[i] = a2.m_data[i];
		}
	}

	~Array()
	{
		for(u32 i = 0; i < m_count; ++i)
		{
			m_data[i].~T();
		}

		delete [] (u8*)m_data;
	}

public:
	void Add(const T &element)
	{
		if( m_count == m_numAllocated )
		{
			m_numAllocated += kAllocInc;
			ReAlloc();
		}

#include "undefnew.h"
		(void) new (&m_data[m_count]) T;
#include "defnew.h"

		m_data[m_count] = element;

		m_count++;
	}

	void Alloc(u32 number)
	{
		if( m_numAllocated < number )
		{
			m_numAllocated = number;
			ReAlloc();
		}
	}

	T &operator[](u32 index)
	{
		return m_data[index];
	}

	const T &operator[](u32 index) const 
	{
		return m_data[index];
	}

	Array &operator=( const Array &a2 )
	{
		Alloc(a2.m_numAllocated);
		
		for(u32 i = m_count; i < a2.m_count; ++i)
		{
#include "undefnew.h"
			(void) new (&m_data[i]) T;
#include "defnew.h"
		}

		m_count = a2.m_count;
		
		for(u32 i = 0; i < m_count; ++i)
		{
			m_data[i] = a2.m_data[i];
		}

		return *this;
	}

	u32 Count() const
	{
		return m_count;
	}

	u32 Allocated() const
	{
		return m_numAllocated;
	}

	T* Data()
	{
		return m_data;
	}

	const T* Data() const
	{
		return m_data;
	}

	void SetCount(u32 count)
	{
		assert(count <= m_numAllocated);
		m_count = count;
	}

	void RemoveIdx(u32 index)
	{
		m_data[index].~T();
		for(u32 i = index; i < m_count - 1; i++)
		{
			memcpy(&m_data[i], &m_data[i+1], sizeof(T));
		}
		m_count--;
	}

	void Remove(const T& element)
	{
		for(int i = (int)m_count - 1; i >= 0; i--)
		{
			if( memcmp(&m_data[i], &element, sizeof(T)) == 0 )
			{
				RemoveIdx((u32)i);
			}
		}
	}

	T RemoveLast()
	{
		assert(m_count > 0);
		m_count--;
		return m_data[m_count];
	}

	void Insert(const T &element, u32 index)
	{
		assert(index <= m_count);
		if( m_count == m_numAllocated )
		{
			m_numAllocated += kAllocInc;
			ReAlloc();
		}

		if( m_count > 0 )
		{
			for(s32 i = (s32)m_count-1; i >= (s32)index; i--)
			{
				memcpy(&m_data[i+1], &m_data[i], sizeof(T));
			}
		}

#include "undefnew.h"
		(void) new (&m_data[index]) T;
#include "defnew.h"
		m_data[index] = element;
		
		m_count++;
	}

	int Find(const T &element) const
	{
		int	returnValue = -1;
		for(u32 i = 0; i < m_count && returnValue < 0; i++)
		{
			if( memcmp(&m_data[i], &element, sizeof(T)) == 0 )
			{
				returnValue = i;
			}
		}

		return returnValue;
	}

	void Clear()
	{
		m_count = 0;
	}

	//	return value for the SortMethod should be
	//		<0 if item 1 goes before item 2
	//		0 if item 1 is equivalent to item 2
	//		>0 if item 1 goes after item 2
	typedef int (*SortMethod)(const void* item1, const void* item2);
	void Sort(SortMethod sortMethod)
	{
		qsort(m_data, m_count, sizeof(T), sortMethod);
	}

private:
	void ReAlloc()
	{
		if( m_data == NULL )
		{
			m_data = (T*)new u8[sizeof(T) * m_numAllocated];
		}
		else
		{
			u8	*newData = new u8[sizeof(T) * m_numAllocated];
			memcpy(newData, m_data, sizeof(T) * m_count);
			delete [] (u8*)m_data;
			m_data = (T*)newData;
		}
	}

protected:
	T	*m_data;
	u32	m_count;
	u32	m_numAllocated;
};