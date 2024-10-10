#include "Sprite.h"

#include "Renderer.h"
#include "DefaultShaderProgram.h"
#include "MaterialData.h"

Sprite::Sprite():
	m_textureHandle(0)
{
}

Sprite::~Sprite()
{
	glDeleteTextures(1, &m_textureHandle);
}

void Sprite::Draw()
{
	/*DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();
	pDefaultShaderProgram->EnableTexture(		m_vertexBuffer.m_vertType & kChannel_Texcoord);
	pDefaultShaderProgram->EnableVertexColor(	m_vertexBuffer.m_vertType & kChannel_Color);
	pDefaultShaderProgram->EnableLighting(		m_vertexBuffer.m_vertType & kChannel_Normal);
	pDefaultShaderProgram->EnableSkinning(		m_vertexBuffer.m_vertType & kChannel_CharacterAnimation_BoneIndices);
	pDefaultShaderProgram->EnableAdditionalColor(false, NULL);*/

	//m_textureData.Bind(MaterialData::eDiffuse);
	{
		glActiveTexture(GL_TEXTURE0 + MaterialData::eDiffuse);
		glBindTexture(GL_TEXTURE_2D, m_textureHandle);
	}

	Renderer::DrawBuffer(kQuads, &m_vertexBuffer);
}

f32 Sprite::GetHeight()
{
	return (f32)m_textureData.m_height;
}

f32 Sprite::GetWidth()
{
	return (f32)m_textureData.m_width;
}

void Sprite::Init(void* textureData, u32 width, u32 height, u32 bpp)
{
	//	reset if we're drawing something else
	{
		delete [] m_textureData.m_pData;
		glDeleteTextures(1, &m_textureHandle);
	}

	//	init texture data
	m_textureData.m_width = width;
	m_textureData.m_height = height;
	m_textureData.m_bpp = bpp;

	if( m_textureData.m_bpp == 24 )
	{
		m_textureData.m_type = GL_RGB;
	}
	else
	{
		m_textureData.m_type = GL_RGBA;
	}

	u8	bytesPerPixel = m_textureData.m_bpp >> 3;
	u32	imageSize = m_textureData.m_width * m_textureData.m_height * bytesPerPixel;
	m_textureData.m_pData = new u8[imageSize];	//	this will get cleaned up by TextureData's destructor
	memcpy(m_textureData.m_pData, textureData, imageSize);

	//	init texture draw
	glGenTextures(1, &m_textureHandle);
	glBindTexture(GL_TEXTURE_2D, m_textureHandle);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//glTexImage2D(GL_TEXTURE_2D, 0, m_bpp >> 3, m_width, m_height, 0, m_type, GL_UNSIGNED_BYTE, textureData.m_pData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, m_textureData.m_width, m_textureData.m_height, 0, m_textureData.m_type, GL_UNSIGNED_BYTE, m_textureData.m_pData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	//	GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	//	GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//	construct the vert data
	m_vertexBuffer.ReInit(4);
	m_vertexBuffer.m_vertType = kChannel_Position | kChannel_Texcoord;
	QuadVert*	vertData = m_vertexBuffer.BeginEdit();
	const f32	kQuadHalfWidth = (f32)m_textureData.m_width * 0.5f;
	const f32	kQuadHalfHeight = (f32)m_textureData.m_height * 0.5f;
	vertData[0].pos = Vector3(-kQuadHalfWidth, kQuadHalfHeight, 0.0f);
	vertData[0].uv = Vector2(0, 1);

	vertData[1].pos = Vector3(-kQuadHalfWidth, -kQuadHalfHeight, 0.0f);
	vertData[1].uv = Vector2(0, 0);

	vertData[2].pos = Vector3(kQuadHalfWidth, -kQuadHalfHeight, 0.0f);
	vertData[2].uv = Vector2(1, 0);

	vertData[3].pos = Vector3(kQuadHalfWidth, kQuadHalfHeight, 0.0f);
	vertData[3].uv = Vector2(1, 1);

	m_vertexBuffer.EndEdit();
}

bool Sprite::IsInitialized()
{
	return m_textureHandle != 0;
}