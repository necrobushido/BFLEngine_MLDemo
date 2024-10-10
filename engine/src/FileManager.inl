template<class T> 
inline FileRef<T>::FileRef() : 
	m_ptr(NULL)
{
}


template<class T> 
inline FileRef<T>::FileRef(const char *filename) : 
	m_ptr(reinterpret_cast<T *>(FileManager::Load(filename)))
{
}


template<class T> 
inline FileRef<T>::FileRef(const FileRef<T> &ref)
{
	if( (m_ptr = ref.m_ptr) != NULL ) 
	{
		g_fileManager->AddRef(m_ptr);
	}
}


template<class T> 
inline FileRef<T>::FileRef(const PlaceholderFileRef &ref)
{
	if( (m_ptr = reinterpret_cast<T *>(*ref)) != NULL )
	{
		g_fileManager->AddRef(*ref);
	}
}


template<class T> 
inline FileRef<T>::FileRef(T *pFileData)
{
	if( (m_ptr = pFileData) != NULL )
	{
		g_fileManager->AddRef(m_ptr);
	}
}


template<class T> 
inline FileRef<T>::~FileRef()
{
	g_fileManager->Release(m_ptr);
}


template<class T> 
inline FileRef<T> & FileRef<T>::operator=(const FileRef &ref)
{
	if( ref.m_ptr != NULL )
	{
		g_fileManager->AddRef(ref.m_ptr);
	}
	g_fileManager->Release(m_ptr);
	m_ptr = ref.m_ptr;
	return *this;
}


template<class T> 
inline FileRef<T> & FileRef<T>::operator=(const PlaceholderFileRef &ref)
{
	if( *ref != NULL ) 
	{
		g_fileManager->AddRef(*ref);
	}
	g_fileManager->Release(m_ptr);
	m_ptr = reinterpret_cast<T *>(*ref);
	return *this;
}


template<class T> 
inline FileRef<T> & FileRef<T>::operator=(T *pFileData)
{
	if( pFileData != NULL ) 
	{
		g_fileManager->AddRef(pFileData);
	}
	g_fileManager->Release(m_ptr);
	m_ptr = pFileData;
	return *this;
}


template<class T> 
inline bool FileRef<T>::operator==(const FileRef &ref) const
{
	return m_ptr == ref.m_ptr;
}


template<class T> 
inline bool FileRef<T>::operator!=(const FileRef &ref) const
{
	return m_ptr != ref.m_ptr;
}


template<class T> 
inline bool FileRef<T>::operator==(const T *pFileData) const
{
	return m_ptr == pFileData;
}


template<class T> 
inline bool FileRef<T>::operator!=(const T *pFileData) const
{
	return m_ptr != pFileData;
}


template<class T> 
inline bool FileRef<T>::operator<(const FileRef &ref) const
{
	return m_ptr < ref.m_ptr;
}


template<class T> 
inline bool FileRef<T>::operator>(const FileRef &ref) const
{
	return m_ptr > ref.m_ptr;
}


template<class T> 
inline T * FileRef<T>::operator->() const
{
	return m_ptr;
}


template<class T> 
inline T * FileRef<T>::operator*() const
{
	return m_ptr;
}


template<class T> 
inline FileRef<T>::operator T*()
{
	return m_ptr;
}


template<class T> 
inline FileRef<T>::operator const T*() const
{
	return m_ptr;
}


template<class T> 
inline FileRef<T>::operator T&()
{
	return *m_ptr;
}


template<class T> 
inline FileRef<T>::operator const T&() const
{
	return *m_ptr;
}


template<class T> 
inline const char * FileRef<T>::GetFilename() const
{
	return g_fileManager->GetFilename(m_ptr);
}

template<class T> 
inline u32 FileRef<T>::GetHash() const
{
	return FileManager::GetFileHash(m_ptr);
}

template<class T> 
inline size_t FileRef<T>::FileSize() const
{
	return g_fileManager->GetFileSize(m_ptr);
}