#pragma once

#include "ModelData.h"
#include "ListMacros.h"
#include "Reference.h"
#include "FileManager.h"

typedef Reference<class Model> ModelRef;

class Model
{
public:	
	static Model *Load(const char *filename);
	static Model *Load(FileRef<ModelData> fileRef);
	void AddRef();
	void Release();

	static ModelRef MakeRef(const char *filename);

	void Draw();
	void DrawWireframe();
	void DrawBase();

	const ModelData *GetData(){ return *m_fileRef; }
	const char *GetFilename(){ return m_fileRef.GetFilename(); }

private:	
	Model(FileRef<ModelData> fileRef, u32 hash);
	~Model();

	// Global hash table of all Models.
	struct HashBin 
	{
		LIST_DECLARE(HashBinSiblings, Model);
	};
	enum { kNumHashBits = 4 };
	static HashBin s_hashTable[1 << kNumHashBits];

	// Our presence in the global hash table.
	u32				m_hash;
	LIST_LINK(HashBinSiblings, Model);

	u32					m_refCount;

	FileRef<ModelData>	m_fileRef;
};

#include "Model.inl"