#pragma once

#include "types.h"
#include "ListMacros.h"
#include "Reference.h"
#include "FileManager.h"
#include "TextureData.h"

typedef Reference<class Texture> TextureRef;

class Texture
{
public:
	static Texture *Load(const char *filename);
	void AddRef();
	void Release();

	static TextureRef MakeRef(const char *filename);

public:
	Texture(TextureData& textureData, u32 hash, const char* filename);
	~Texture();

public:
	void Bind(u32 channel);		//	channels correspond to MaterialData channel enum
	const char *GetFilename();

public:
	// Global hash table of all Textures.
	struct HashBin 
	{
		LIST_DECLARE(HashBinSiblings, Texture);
	};
	enum { kNumHashBits = 4 };
	static HashBin s_hashTable[1 << kNumHashBits];

	// Our presence in the global hash table.
	u32				m_hash;
	LIST_LINK(HashBinSiblings, Texture);

	u32				m_refCount;

	char			m_filename[255];

public:
	u32	m_handle;
	u32	m_width;
	u32	m_height;
	u32	m_bpp;
	u32	m_type;
};

#include "Texture.inl"