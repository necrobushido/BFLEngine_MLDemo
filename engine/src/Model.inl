inline Model *Model::Load(const char *filename)
{
	FileRef<ModelData>	fileRef = g_fileManager->MakeRef(filename);
	return Load(fileRef);
}

inline void Model::AddRef()
{
	++m_refCount;
}

inline void Model::Release()
{
	if( --m_refCount == 0 ) 
	{
		delete this;
	}
}

inline ModelRef Model::MakeRef(const char *filename)
{
	//	keeps the reference count straight.
	FileRef<ModelData>	fileRef = g_fileManager->MakeRef(filename);
	ModelRef	ref(Load(fileRef));
	ref->Release();
	return ref;
}