inline LiveTextFont *LiveTextFont::Load(const char *filename, u32 pixelSize)
{
	FileRef<u8>	fileRef = g_fileManager->MakeRef(filename);
	return Load(fileRef, pixelSize);
}

inline void LiveTextFont::AddRef()
{
	++m_refCount;
}

inline void LiveTextFont::Release()
{
	if( --m_refCount == 0 ) 
	{
		delete this;
	}
}

inline LiveTextFontRef LiveTextFont::MakeRef(const char *filename, u32 pixelSize)
{
	//	keeps the reference count straight.
	FileRef<u8>		fileRef = g_fileManager->MakeRef(filename);
	LiveTextFontRef	ref(Load(fileRef, pixelSize));
	ref->Release();
	return ref;
}