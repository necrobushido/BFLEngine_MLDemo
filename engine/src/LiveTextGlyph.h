#pragma once

#include "dataTypes.h"
#include "VertexBufferBase.h"

class LiveTextGlyph
{
public:
	LiveTextGlyph(u8* textureData, u32 textureWidth, u32 textureHeight, u32 glyphWidth, u32 glyphHeight, u32 left, u32 top);
	~LiveTextGlyph();

public:
	void Draw(const Color4 &color);

protected:
	void InitDraw(u8* glyphTextureData, u32 glyphTextureWidth, u32 glyphTextureHeight);

protected:
	VertexBufferBase	m_vertBuffer;
	u32					m_textureHandle;
	u32					m_vao;

public:	
	u32					m_glyphHeight;
	u32					m_glyphWidth;
	s32					m_top;
	s32					m_left;
};