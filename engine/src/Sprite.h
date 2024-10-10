#pragma once

#include "Vector3.h"
#include "Vector2.h"
#include "VertexBuffer.h"
#include "TextureData.h"

class Sprite
{
public:
	Sprite();
	virtual ~Sprite();
	
public:
	//	Sprite draws with the center of the image as the registration point
	void Draw();

	f32 GetHeight();
	f32 GetWidth();

	void Init(void* textureData, u32 width, u32 height, u32 bpp);

	bool IsInitialized();

protected:
	struct QuadVert
	{
		Vector3	pos;
		Vector2	uv;
	};	

protected:
	VertexBuffer<QuadVert>	m_vertexBuffer;
	TextureData				m_textureData;
	u32						m_textureHandle;
};