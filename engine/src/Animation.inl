inline Animation *Animation::Load(const char *filename)
{
	FileRef<AnimationData>	fileRef = g_fileManager->MakeRef(filename);
	return Load(fileRef);
}

inline void Animation::AddRef()
{
	++m_refCount;
}

inline void Animation::Release()
{
	if( --m_refCount == 0 ) 
	{
		delete this;
	}
}

inline AnimationRef Animation::MakeRef(const char *filename)
{
	//	keeps the reference count straight.
	FileRef<AnimationData>	fileRef = g_fileManager->MakeRef(filename);
	AnimationRef	ref(Load(fileRef));
	ref->Release();
	return ref;
}