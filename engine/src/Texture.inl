inline void Texture::AddRef()
{
	++m_refCount;
}

inline void Texture::Release()
{
	if( --m_refCount == 0 ) 
	{
		delete this;
	}
}

inline TextureRef Texture::MakeRef(const char *filename)
{	
	//	keeps the reference count straight.
	TextureRef	ref(Load(filename));
	ref->Release();
	return ref;
}

inline const char *Texture::GetFilename()
{ 
	return m_filename; 
}