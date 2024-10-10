#include "Model.h"
#include "Hash.h"

Model::HashBin Model::s_hashTable[1 << kNumHashBits];

Model *Model::Load(FileRef<ModelData> fileRef)
{
	// See if the Model already exists for the given file.
	// If it does, then just return that one after incrementing its
	// reference count.
	//const u32	hash = JenkinsHash(u32(*fileRef));
	const u32	hash = JenkinsHash((*fileRef));
	const u32	hashBinIndex = hash & ((1 << kNumHashBits) - 1);
	Model		*pModel;
	for(pModel = LIST_HEAD(&s_hashTable[hashBinIndex], HashBinSiblings); (pModel != NULL) && (pModel->m_fileRef != fileRef); pModel = LIST_NEXT(pModel, HashBinSiblings)) { }
	if( pModel != NULL ) 
	{
		pModel->m_refCount++;
		return pModel;
	}
	
	// The Model does not exist yet.  Create it here.
	pModel = new Model(fileRef, hash);
	//Memory::SetPtrDescription(pModel, fileRef.GetFilename());
	return pModel;
}

Model::Model(FileRef<ModelData> fileRef, u32 hash): 
	m_hash(hash), 
	m_refCount(1),
	m_fileRef(fileRef)
{
	// Add the Model to a global hash table.
	const unsigned hashBinIndex = m_hash & ((1 << kNumHashBits) - 1);
	LIST_ADD(&s_hashTable[hashBinIndex], HashBinSiblings, this);

	//
	//DebugPrintf("initializing Model : %s\n", fileRef.GetFilename());
	m_fileRef->Init();
}

Model::~Model()
{
	// Remove the Model from the global hash table.
	const unsigned hashBinIndex = m_hash & ((1 << kNumHashBits) - 1);
	LIST_REMOVE(&s_hashTable[hashBinIndex], HashBinSiblings, this);

	//
	m_fileRef->Deinit();
}

void Model::Draw()
{
	m_fileRef->Draw();
}

void Model::DrawWireframe()
{
	m_fileRef->DrawWireframe();
}

void Model::DrawBase()
{
	m_fileRef->DrawBase();
}