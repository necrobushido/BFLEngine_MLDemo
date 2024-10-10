#pragma once

#include "LiveTextFont.h"
#include "Vector3.h"
#include "Color.h"

class PerspectiveFont
{
public:
	enum eJustify
	{
		kJustify_Center,
		kJustify_Left,
		kJustify_Right
	};

public:
	PerspectiveFont(const char* filename);
	~PerspectiveFont();

public:
	void Draw(const char* message, const Vector3& position, eJustify justification = kJustify_Center);
	void Draw(const char* message, const Vector3& position, const Color4& color, f32 textScale, eJustify justification = kJustify_Center);

private:
	LiveTextFontRef	m_fontRef;
	f32				m_baseTextScale;
};