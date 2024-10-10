#pragma once

#include "AnimationData.h"
#include "ListMacros.h"
#include "Reference.h"
#include "FileManager.h"

typedef Reference<class Animation> AnimationRef;

class Animation
{
public:	
	static Animation *Load(const char *filename);
	static Animation *Load(FileRef<AnimationData> fileRef);
	void AddRef();
	void Release();

	static AnimationRef MakeRef(const char *filename);

	const AnimationData *GetData(){ return *m_fileRef; }

private:	
	Animation(FileRef<AnimationData> fileRef, u32 hash);
	~Animation();

	// Global hash table of all Animations.
	struct HashBin 
	{
		LIST_DECLARE(HashBinSiblings, Animation);
	};
	enum { kNumHashBits = 4 };
	static HashBin s_hashTable[1 << kNumHashBits];

	// Our presence in the global hash table.
	u32						m_hash;
	LIST_LINK(HashBinSiblings, Animation);

	u32						m_refCount;

	//	data
	FileRef<AnimationData>	m_fileRef;
};

#include "Animation.inl"