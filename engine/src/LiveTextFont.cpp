#include "LiveTextFont.h"
#include "Hash.h"
#include "Renderer.h"
#include "LiveTextGlyph.h"

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_STROKER_H
#include "freetype/ftmodapi.h"

static void* FT_Alloc(FT_Memory memory, long size)
{
	//u8* alloc = new u8[size];
	//Memory::SetPtrDescription(alloc, "FreeType");

	//	supposedly default functionality
	void*	alloc = malloc(size);

	return alloc;
}

static void* FT_Realloc(FT_Memory memory, long cur_size, long new_size, void* block)
{
	//void* result = nullptr;

	//if( block == nullptr )
	//{
	//	result = FT_Alloc(memory, new_size);
	//}
	//else
	//{
	//	result = new u8[new_size];
	//	//Memory::SetPtrDescription(result, "FreeType");
	//	memcpy(result, block, new_size);
	//	delete [] (u8*)block;
	//}

	//return result;

	//	supposedly default functionality
	return realloc(block, new_size);
}

static void FT_Free(FT_Memory memory, void* block)
{
	//delete [] (u8*)block;

	//	supposedly default functionality
	free(block);
} 

LiveTextFont::HashBin LiveTextFont::s_hashTable[1 << kNumHashBits];

FT_Library	s_freeTypeLibrary;
FT_Memory	s_freeTypeMemory;

//	static
void LiveTextFont::InitLibrary()
{
	s_freeTypeMemory = (FT_Memory)new u8[sizeof(*s_freeTypeMemory)];
	s_freeTypeMemory->user = 0;
	s_freeTypeMemory->alloc = FT_Alloc;
	s_freeTypeMemory->realloc = FT_Realloc;
	s_freeTypeMemory->free = FT_Free;
	FT_Error	error = FT_New_Library(s_freeTypeMemory, &s_freeTypeLibrary);
	if( !error ) 
	{
		FT_Add_Default_Modules(s_freeTypeLibrary);
	}

	//FT_Error	error = FT_Init_FreeType(&s_freeTypeLibrary);
	AssertMsg(!error, "an error occurred during FT_Library initialization\n");
}

void LiveTextFont::ShutdownLibrary()
{
	FT_Error	error = FT_Done_Library(s_freeTypeLibrary);
	AssertMsg(error == 0, "Shutdown() failed, error code: %d.", error);
	delete [] (u8*)s_freeTypeMemory;

	//FT_Done_FreeType(s_freeTypeLibrary);
}

//
LiveTextFont *LiveTextFont::Load(FileRef<u8> fileRef, u32 pixelSize)
{
	// See if the Font already exists for the given file.
	// If it does, then just return that one after incrementing its
	// reference count.
	const u32		hash = JenkinsHash(u64(*fileRef));
	const u32		hashBinIndex = hash & ((1 << kNumHashBits) - 1);
	LiveTextFont	*pFont;
	for(pFont = LIST_HEAD(&s_hashTable[hashBinIndex], HashBinSiblings); (pFont != NULL) && (pFont->m_fileRef != fileRef) && (pFont->m_pixelSize != pixelSize); pFont = LIST_NEXT(pFont, HashBinSiblings)) { }
	if( pFont != NULL ) 
	{
		pFont->m_refCount++;
		return pFont;
	}
	
	// The Font does not exist yet.  Create it here.
	pFont = new LiveTextFont(fileRef, hash, pixelSize);
	//Memory::SetPtrDescription(pFont, fileRef.GetFilename());
	return pFont;
}

LiveTextFont::LiveTextFont(FileRef<u8> fileRef, u32 hash, u32 pixelSize): 
	m_hash(hash), 
	m_refCount(1),
	m_fileRef(fileRef),
	m_pixelSize(pixelSize),
	//m_pixelSize(16*64),
	m_strokeSize(0.0f),
	m_stroke(NULL),
	m_glyphs(NULL)
{
	// Add the Font to a global hash table.
	const unsigned hashBinIndex = m_hash & ((1 << kNumHashBits) - 1);
	LIST_ADD(&s_hashTable[hashBinIndex], HashBinSiblings, this);

	//
	DebugPrintf("initializing Font : %s\n", fileRef.GetFilename());
	
	Init();
}

LiveTextFont::~LiveTextFont()
{
	// Remove the Font from the global hash table.
	const unsigned hashBinIndex = m_hash & ((1 << kNumHashBits) - 1);
	LIST_REMOVE(&s_hashTable[hashBinIndex], HashBinSiblings, this);

	for(u32 glyphIdx = 0; glyphIdx < m_glyphCount; glyphIdx++)
	{
		if( m_glyphs[glyphIdx] )
		{
			delete m_glyphs[glyphIdx];
		}
	}

	delete [] m_glyphs;

	if( m_stroke )
	{
		FT_Stroker_Done(m_stroke);
	}

	FT_Done_Face(m_face);
}

void LiveTextFont::Init()
{
	FT_Error	error;

	// set up the ft face
	error = FT_New_Memory_Face(s_freeTypeLibrary, (FT_Byte*)(*m_fileRef), (FT_Long)(m_fileRef.FileSize()), 0, &m_face);
	AssertMsg(error == 0, "LiveTextFont::Init : FT_New_Memory_Face() failed, face '%s', error: %d.\n", m_fileRef.GetFilename(), error );
	
	// set the styling
	error = FT_Set_Char_Size(m_face, 0, m_pixelSize, 72, 72);
	//error = FT_Set_Pixel_Sizes(m_face, 0, m_pixelSize);
	AssertMsg(error == 0, "LiveTextFont::Init() : FT_Set_Pixel_Sizes() failed, face '%s', error: %d.\n", m_fileRef.GetFilename(), error);

	/*if( m_strokeSize > 0.0f )
	{
		error = FT_Stroker_New(s_freeTypeLibrary, &m_stroke);
		AssertMsg(error == 0, "LiveTextFont::Init() : FT_Stroker_New() failed, face '%s', error: %d.\n", m_fileRef.GetFilename(), error );

		FT_Stroker_Set(m_stroke, FT_Fixed(m_strokeSize * f32( 1 << 6 )), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

		m_strokeGlyphs = wfnew ( g_engineHeap ) wfFontGlyph*[ m_face->num_glyphs ];
		wfHeap::SetPtrDesc( m_strokeGlyphs, "stroke glyph ptr array" );
		memset( m_strokeGlyphs, 0, sizeof( wfFontGlyph* ) * m_face->num_glyphs );
	}*/

	//
	m_fontHeight = m_face->size->metrics.height >> 6;

	// set up glyph array
	//m_glyphCount = m_face->num_glyphs;	//	normal
	m_glyphCount = 128;						//	just ASCII for now
	m_glyphs = new LiveTextGlyph*[m_glyphCount];
	//Memory::SetPtrDescription(m_glyphs, "glyph ptr array");
	memset(m_glyphs, 0, sizeof(LiveTextGlyph*) * m_glyphCount);

	//	cache glyphs
	for(u32 glyphIdx = 0; glyphIdx < m_glyphCount; glyphIdx++)
	{
		FT_ULong	charcode = glyphIdx;	//	UTF-32 character code
		error = FT_Load_Char(m_face, charcode, FT_LOAD_RENDER);
		//error = FT_Load_Glyph(m_face, glyphIdx, 0);
		AssertMsg(error == 0, "an error occurred during FT_Load_Char\n");

		m_glyphs[glyphIdx] = new LiveTextGlyph(	m_face->glyph->bitmap.buffer, 
												m_face->glyph->bitmap.width, 
												m_face->glyph->bitmap.rows, 
												m_face->glyph->advance.x >> 6, 
												m_face->glyph->advance.y >> 6, 
												m_face->glyph->bitmap_left,
												m_face->glyph->bitmap_top - m_face->glyph->bitmap.rows );
	}
}

bool LiveTextFont::HasKerning()
{
	FT_Bool		hasKerning = FT_HAS_KERNING(m_face);
	return hasKerning != 0;
}

Vector2 LiveTextFont::GetKerning(u32 leftGlyphIdx, u32 rightGlyphIdx)
{
	FT_Vector	delta; 
	FT_Get_Kerning(m_face, leftGlyphIdx, rightGlyphIdx, FT_KERNING_DEFAULT, &delta);

	Vector2		returnValue;
	returnValue.x = (f32)(delta.x >> 6);
	returnValue.y = (f32)(delta.y >> 6);

	return returnValue;
}