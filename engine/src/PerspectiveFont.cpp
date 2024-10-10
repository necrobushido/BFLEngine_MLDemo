#include "PerspectiveFont.h"
//#include "MaterialData.h"

#include "LiveTextGlyph.h"
#include "Mtx44.h"
#include "Renderer.h"
#include "CameraBase.h"

namespace
{
	const u32	kRows = 16;
	const u32	kColumns = 16;
	const f32	kCharWidth = 1.0f;
	const f32	kCharHeight = 1.0f;
}

PerspectiveFont::PerspectiveFont(const char* filename)
{
	u32	pixelSize = 1 << 10;
	m_fontRef = LiveTextFont::MakeRef(filename, pixelSize);

	u32				utf32Char = 'B';	//	pulled out of my ass as a character to choose as the standard
	LiveTextGlyph*	pTestGlyph = m_fontRef->GetGlyph(utf32Char);

	m_baseTextScale = 1.0f / pTestGlyph->m_glyphHeight;
}

PerspectiveFont::~PerspectiveFont()
{
}

void PerspectiveFont::Draw(const char* message, const Vector3& position, eJustify justification)
{
	Color4	color(1.0f, 1.0f, 1.0f, 1.0f);
	Draw(message, position, color, 1.0f, justification);
}

void PerspectiveFont::Draw(const char* message, const Vector3& position, const Color4& color, f32 textScale, eJustify justification)
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

	Mtx44	transform = g_camera->GetInvViewMatrix();
	Vector3	right = transform.GetRight();
	transform.SetScale(m_baseTextScale * textScale);

	Vector3	basePos = position;
	Vector3	currentPos = Vector3::ZERO;
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

		Vector3	transformedPos;
		transform.MultiplyVec43(localPos, transformedPos, false);
		transformedPos += basePos;
		transform.SetTranslation3(transformedPos);
		Renderer::SetModelMatrix(&transform);
		
		glyph->Draw(color);
		currentPos.x += (f32)glyph->m_glyphWidth;

		prevCharIdx = charIdx;
		charIdx++;
	}
}