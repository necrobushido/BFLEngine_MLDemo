#include "OrthoFont.h"

#include "LiveTextGlyph.h"
#include "Mtx44.h"
#include "Renderer.h"

namespace
{
	const u32	kRows = 16;
	const u32	kColumns = 16;
	const f32	kCharWidth = 16.0f;
	const f32	kCharHeight = 16.0f;
}

OrthoFont::OrthoFont(const char *filename, u32 pixelSize)
{
	m_fontRef = LiveTextFont::MakeRef(filename, pixelSize);
}

OrthoFont::~OrthoFont()
{
}

void OrthoFont::Draw(const char *message, const Vector3 &position, eJustify justification)
{
	Color4	color(1.0f, 1.0f, 1.0f, 1.0f);
	Draw(message, position, color, justification);
}

void OrthoFont::Draw(const char *message, const Vector3 &position, const Color4 &color, eJustify justification)
{
	u32	messageLength = (u32)strlen(message);
	u32	charIdx = 0;
	f32	messageWidth = 0;
	while(message[charIdx] != '\0')
	{
		u32				utf32Char = message[charIdx];
		LiveTextGlyph*	glyph = m_fontRef->GetGlyph(utf32Char);
		messageWidth += (f32)glyph->m_glyphWidth;

		charIdx++;
	}

	f32	currentQuadXPos;
	switch(justification)
	{
	case kJustify_Left:
		currentQuadXPos = 0.0f;
		break;

	case kJustify_Right:
		currentQuadXPos = -messageWidth;
		break;

	default:
	case kJustify_Center:
		currentQuadXPos = -messageWidth / 2;
		break;
	}

	Mtx44	transform;
	transform.Identity();
	Vector3	currentPos = position;
	currentPos.x += currentQuadXPos;

	charIdx = 0;
	u32	prevCharIdx = 0;
	while(message[charIdx] != '\0')
	{
		u32				utf32Char = message[charIdx];
		LiveTextGlyph*	glyph = m_fontRef->GetGlyph(utf32Char);

		Vector3			localPos = currentPos;
		localPos.x += glyph->m_left;
		localPos.y += glyph->m_top;

		if( m_fontRef->HasKerning() )
		{
			localPos.x += m_fontRef->GetKerning(prevCharIdx, utf32Char).x;
		}

		transform.SetTranslation3(localPos);
		Renderer::SetModelMatrix(&transform);
		
		glyph->Draw(color);
		currentPos.x += (f32)glyph->m_glyphWidth;

		prevCharIdx = charIdx;
		charIdx++;
	}
}

f32 OrthoFont::GetTextHeight()
{
	/*u32			utf32Char = 'A';
	GlyphData	*glyph = m_fontRef->GetGlyph(utf32Char);
	return (f32)glyph->glyphHeight;*/

	//return kCharHeight;

	return (f32)m_fontRef->GetHeight();
}