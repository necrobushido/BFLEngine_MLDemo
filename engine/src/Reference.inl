template<class T> inline Reference<T>::Reference() : m_ptr(0)
{
}


template<class T> inline Reference<T>::Reference(const Reference &ref)
{
	if( (m_ptr = ref.m_ptr) != 0 ) 
	{
		m_ptr->AddRef();
	}
}


template<class T> inline Reference<T>::Reference(T *pObj)
{
	if( (m_ptr = pObj) != 0 )
	{
		pObj->AddRef();
	}
}


template<class T> inline Reference<T>::~Reference()
{
	if( m_ptr != 0 ) 
	{
		m_ptr->Release();
	}
}


template<class T> inline Reference<T> &Reference<T>::operator=(const Reference &ref)
{
	if( ref.m_ptr != 0 ) 
	{
		ref.m_ptr->AddRef();
	}
	if( m_ptr != 0 ) 
	{
		m_ptr->Release();
	}
	m_ptr = ref.m_ptr;
	return *this;
}


template<class T> inline Reference<T> &Reference<T>::operator=(T *pObj)
{
	if( pObj != 0 ) 
	{
		pObj->AddRef();
	}
	if( m_ptr != 0 ) 
	{
		m_ptr->Release();
	}
	m_ptr = pObj;
	return *this;
}


template<class T> inline bool Reference<T>::operator==(const Reference &ref) const
{
	return m_ptr == ref.m_ptr;
}


template<class T> inline bool Reference<T>::operator!=(const Reference &ref) const
{
	return m_ptr != ref.m_ptr;
}


template<class T> inline bool Reference<T>::operator==(const T *pObj) const
{
	return m_ptr == pObj;
}


template<class T> inline bool Reference<T>::operator!=(const T *pObj) const
{
	return m_ptr != pObj;
}


template<class T> inline bool Reference<T>::operator<(const Reference &ref) const
{
	return m_ptr < ref.m_ptr;
}


template<class T> inline bool Reference<T>::operator>(const Reference &ref) const
{
	return m_ptr > ref.m_ptr;
}


template<class T> inline T *Reference<T>::operator->() const
{
	return m_ptr;
}


template<class T> inline T *Reference<T>::operator*() const
{
	return m_ptr;
}


template<class T> inline Reference<T>::operator T*()
{
	return m_ptr;
}


template<class T> inline Reference<T>::operator const T*() const
{
	return m_ptr;
}


template<class T> inline Reference<T>::operator T&()
{
	return *m_ptr;
}


template<class T> inline Reference<T>::operator const T&() const
{
	return *m_ptr;
}
