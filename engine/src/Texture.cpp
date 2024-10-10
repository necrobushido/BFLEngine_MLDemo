#include "Texture.h"

#include "Hash.h"

Texture::HashBin Texture::s_hashTable[1 << kNumHashBits];

Texture *Texture::Load(const char* filename)
{
	// See if the Texture already exists for the given file.
	// If it does, then just return that one after incrementing its
	// reference count.
	const u32	hash = FilenameHash(filename);
	const u32	hashBinIndex = hash & ((1 << kNumHashBits) - 1);
	Texture		*pTexture;
	for(pTexture = LIST_HEAD(&s_hashTable[hashBinIndex], HashBinSiblings); (pTexture != NULL) && (strcmp(pTexture->m_filename, filename) != 0); pTexture = LIST_NEXT(pTexture, HashBinSiblings)) { }
	if( pTexture != NULL ) 
	{
		pTexture->m_refCount++;
		return pTexture;
	}
	
	// The Texture does not exist yet.  Create it here.
	TextureData	textureData;
	textureData.Load(filename);

	pTexture = new Texture(textureData, hash, filename);
	//Memory::SetPtrDescription(pTexture, filename);
	return pTexture;
}

Texture::Texture(TextureData& textureData, u32 hash, const char* filename): 
	m_hash(hash), 
	m_refCount(1),
	m_handle(0)
{
	// Add the Texture to a global hash table.
	const unsigned hashBinIndex = m_hash & ((1 << kNumHashBits) - 1);
	LIST_ADD(&s_hashTable[hashBinIndex], HashBinSiblings, this);

	m_width = textureData.m_width;
	m_height = textureData.m_height;
	m_bpp = textureData.m_bpp;
	m_type = textureData.m_type;

	glGenTextures(1, &m_handle);
	glBindTexture(GL_TEXTURE_2D, m_handle);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//glTexImage2D(GL_TEXTURE_2D, 0, m_bpp >> 3, m_width, m_height, 0, m_type, GL_UNSIGNED_BYTE, textureData.m_pData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, m_width, m_height, 0, m_type, GL_UNSIGNED_BYTE, textureData.m_pData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	//	GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	//	GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	strcpy(m_filename, filename);
}

Texture::~Texture()
{
	// Remove the Texture from the global hash table.
	const unsigned hashBinIndex = m_hash & ((1 << kNumHashBits) - 1);
	LIST_REMOVE(&s_hashTable[hashBinIndex], HashBinSiblings, this);

	glDeleteTextures(1, &m_handle);	//	a value of 0 for m_handle is ignored silently
}

void Texture::Bind(u32 channel)
{
	Assert(channel < 32);	//	this looked correct from the header
	glActiveTexture(GL_TEXTURE0 + channel);
	glBindTexture(GL_TEXTURE_2D, m_handle);
	//glBindSampler(GL_TEXTURE0 + channel, linearFiltering);
}