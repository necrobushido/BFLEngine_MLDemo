#pragma once

#include "ListMacros.h"
#include "Reference.h"
#include "FileManager.h"
#include "Vector2.h"

struct FT_FaceRec_;
struct FT_StrokerRec_;

class LiveTextGlyph;

typedef Reference<class LiveTextFont> LiveTextFontRef;

class LiveTextFont
{
public:
	static void InitLibrary();
	static void ShutdownLibrary();

public:	
	static LiveTextFont *Load(const char *filename, u32 pixelSize);
	static LiveTextFont *Load(FileRef<u8> fileRef, u32 pixelSize);
	void AddRef();
	void Release();

	static LiveTextFontRef MakeRef(const char *filename, u32 pixelSize);

	const LiveTextGlyph *GetGlyph(u32 glyphIndex) const { return m_glyphs[glyphIndex]; }
	LiveTextGlyph *GetGlyph(u32 glyphIndex){ return m_glyphs[glyphIndex]; }
	u32 GetGlyphCount(){ return m_glyphCount; }
	u32 GetHeight(){ return m_fontHeight; }
	bool HasKerning();
	Vector2 GetKerning(u32 leftGlyphIdx, u32 rightGlyphIdx);

private:	
	LiveTextFont(FileRef<u8> fileRef, u32 hash, u32 pixelSize);
	~LiveTextFont();

private:
	void Init();

private:
	// Global hash table of all Fonts.
	struct HashBin 
	{
		LIST_DECLARE(HashBinSiblings, LiveTextFont);
	};
	enum { kNumHashBits = 4 };
	static HashBin s_hashTable[1 << kNumHashBits];

	// Our presence in the global hash table.
	u32				m_hash;
	LIST_LINK(HashBinSiblings, LiveTextFont);

	u32					m_refCount;

	FileRef<u8>			m_fileRef;

protected:
	FT_FaceRec_*	m_face;
	FT_StrokerRec_*	m_stroke;
	const u32		m_pixelSize;
	const f32		m_strokeSize;

	LiveTextGlyph**	m_glyphs;
	u32				m_glyphCount;
	u32				m_fontHeight;
};

#include "LiveTextFont.inl"