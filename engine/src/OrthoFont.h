#pragma once

#include "LiveTextFont.h"
#include "Vector3.h"
#include "Color.h"

class OrthoFont
{
public:
	enum eJustify
	{
		kJustify_Center,
		kJustify_Left,
		kJustify_Right
	};

public:
	OrthoFont(const char *filename, u32 pixelSize);
	~OrthoFont();

public:
	void Draw(const char *message, const Vector3 &position, eJustify justification = kJustify_Center);
	void Draw(const char *message, const Vector3 &position, const Color4 &color, eJustify justification = kJustify_Center);
	f32 GetTextHeight();

private:
	LiveTextFontRef	m_fontRef;
};