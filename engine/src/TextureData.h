#pragma once

#include "types.h"

class TextureData
{
public:
	TextureData();
	~TextureData();

public:
	void FlipImageVertically();
	void FlipImageHorizontally();
	void Load(const char* filename);

public:
	u32	m_width;
	u32	m_height;
	u32	m_bpp;
	u32	m_type;
	u8	*m_pData;
};