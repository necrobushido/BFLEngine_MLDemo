#include "Animation.h"
#include "Hash.h"

Animation::HashBin Animation::s_hashTable[1 << kNumHashBits];

Animation *Animation::Load(FileRef<AnimationData> fileRef)
{
	// See if the Animation already exists for the given file.
	// If it does, then just return that one after incrementing its
	// reference count.
	//const u32	hash = JenkinsHash(u32(*fileRef));
	const u32	hash = JenkinsHash((*fileRef));
	const u32	hashBinIndex = hash & ((1 << kNumHashBits) - 1);
	Animation	*pAnimation;
	for(pAnimation = LIST_HEAD(&s_hashTable[hashBinIndex], HashBinSiblings); (pAnimation != NULL) && (pAnimation->m_fileRef != fileRef); pAnimation = LIST_NEXT(pAnimation, HashBinSiblings)) { }
	if( pAnimation != NULL ) 
	{
		pAnimation->m_refCount++;
		return pAnimation;
	}
	
	// The Animation does not exist yet.  Create it here.
	pAnimation = new Animation(fileRef, hash);
	//Memory::SetPtrDescription(pAnimation, fileRef.GetFilename());
	return pAnimation;
}

Animation::Animation(FileRef<AnimationData> fileRef, u32 hash): 
	m_hash(hash), 
	m_refCount(1),
	m_fileRef(fileRef)
{
	// Add the Animation to a global hash table.
	const unsigned hashBinIndex = m_hash & ((1 << kNumHashBits) - 1);
	LIST_ADD(&s_hashTable[hashBinIndex], HashBinSiblings, this);

	//	
	m_fileRef->Init();
}


Animation::~Animation()
{
	// Remove the Animation from the global hash table.
	const unsigned hashBinIndex = m_hash & ((1 << kNumHashBits) - 1);
	LIST_REMOVE(&s_hashTable[hashBinIndex], HashBinSiblings, this);

	//
	m_fileRef->Deinit();
}
