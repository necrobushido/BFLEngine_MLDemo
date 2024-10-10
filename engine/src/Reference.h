#pragma once

//	T needs to implement functions:
//		void AddRef();
//		void Release();
template<class T> class Reference 
{
public:
	Reference();
	Reference(const Reference &ref);
	Reference(T *pObj);
	~Reference();
	
	// Assignment
	Reference &operator=(const Reference &ref);
	Reference &operator=(T *pObj);

	// Equality
	bool operator==(const Reference &ref) const;
	bool operator!=(const Reference &ref) const;
	bool operator==(const T *pObj) const;
	bool operator!=(const T *pObj) const;

	// Sorting
	bool operator<(const Reference &ref) const;
	bool operator>(const Reference &ref) const;

	// Dereferencing
	T *operator->() const;
	T *operator*() const;

	// Casting operators.  These allow a Reference<T> to be passed
	// by value into a function expecting a T* or a T&.
	operator T*();
	operator const T*() const;
	operator T&();
	operator const T&() const;
	
protected:
	T *m_ptr;
};

#include "Reference.inl"
