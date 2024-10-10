#include "LiveTextGlyph.h"

#include "Renderer.h"
#include "MaterialData.h"
#include "DefaultShaderProgram.h"

inline u32 NextPowerOf2(u32 number)
{
	number--;
	number |= number >> 1;
	number |= number >> 2;
	number |= number >> 4;
	number |= number >> 8;
	number |= number >> 16;
	number++;

	if( number < 2 )
		number = 2;

	return number;
}

LiveTextGlyph::LiveTextGlyph(u8* textureData, u32 textureWidth, u32 textureHeight, u32 glyphWidth, u32 glyphHeight, u32 left, u32 top):
	m_textureHandle(0),
	m_vao(0)
{
	m_glyphWidth = glyphWidth;
	m_glyphHeight = textureHeight;
	m_left = left;
	m_top = top;

	//	write texture data
	u32	roundedTextureWidth = NextPowerOf2(textureWidth);
	u32	roundedTextureHeight = NextPowerOf2(textureHeight);
	u8*	glyphTextureData = new u8[roundedTextureWidth * roundedTextureHeight * 2];
	for(u32 texY = 0; texY < roundedTextureHeight; texY++)
	{
		for(u32 texX = 0; texX < roundedTextureWidth; texX++)
		{
			u8	pixelValue = 0;
			if( texY < (u32)textureHeight && 
				texX < (u32)textureWidth )
			{
				pixelValue = textureData[texX + textureWidth * texY];
			}
			glyphTextureData[(texX + roundedTextureWidth * texY) * 2 + 0] = pixelValue;
			glyphTextureData[(texX + roundedTextureWidth * texY) * 2 + 1] = pixelValue;
		}
	}

	//	write vert data
	m_vertBuffer.m_vertCount = 4;
	m_vertBuffer.m_vertType = kChannel_Position | kChannel_Texcoord;
	u32		vertSize = GetVertSize(m_vertBuffer.m_vertType);
	m_vertBuffer.m_bufferIndex = 0;
	f32*	vertDataBuffer = new f32[vertSize * m_vertBuffer.m_vertCount / sizeof(f32)];	//	can I dump this after InitDraw also?
	m_vertBuffer.m_pData = vertDataBuffer;

	f32 uvS = (f32)textureWidth / (f32)roundedTextureWidth;		//	non-rounded value divided by rounded value
	f32	uvT = (f32)textureHeight / (f32)roundedTextureHeight;	//	non-rounded value divided by rounded value

	int	bufferIndex = 0;
	//	1st position
	vertDataBuffer[bufferIndex++] = 0.0f;
	vertDataBuffer[bufferIndex++] = (f32)textureHeight;
	vertDataBuffer[bufferIndex++] = 0.0f;
	//	1st uv
	vertDataBuffer[bufferIndex++] = 0.0f;
	vertDataBuffer[bufferIndex++] = 0.0f;

	//	2nd position
	vertDataBuffer[bufferIndex++] = 0.0f;
	vertDataBuffer[bufferIndex++] = 0.0f;
	vertDataBuffer[bufferIndex++] = 0.0f;
	//	2nd uv
	vertDataBuffer[bufferIndex++] = 0.0f;
	vertDataBuffer[bufferIndex++] = uvT;

	//	3rd position
	vertDataBuffer[bufferIndex++] = (f32)textureWidth;
	vertDataBuffer[bufferIndex++] = 0.0f;
	vertDataBuffer[bufferIndex++] = 0.0f;
	//	3rd uv
	vertDataBuffer[bufferIndex++] = uvS;
	vertDataBuffer[bufferIndex++] = uvT;

	//	4th position
	vertDataBuffer[bufferIndex++] = (f32)textureWidth;
	vertDataBuffer[bufferIndex++] = (f32)textureHeight;
	vertDataBuffer[bufferIndex++] = 0.0f;
	//	4th uv
	vertDataBuffer[bufferIndex++] = uvS;
	vertDataBuffer[bufferIndex++] = 0.0f;

	//	prepare for drawing
	InitDraw(glyphTextureData, roundedTextureWidth, roundedTextureHeight);
	delete [] glyphTextureData;	//	don't need to hold onto this any longer
}

LiveTextGlyph::~LiveTextGlyph()
{
	glDeleteTextures(1, &m_textureHandle);
	glDeleteBuffers(1, &m_vertBuffer.m_bufferIndex);
	glDeleteVertexArrays(1, &m_vao);

	delete [] (u8*)m_vertBuffer.m_pData;
}

void LiveTextGlyph::InitDraw(u8* glyphTextureData, u32 glyphTextureWidth, u32 glyphTextureHeight)
{
	//	prepare texture
	glGenTextures(1, &m_textureHandle);
	glBindTexture(GL_TEXTURE_2D, m_textureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glyphTextureWidth, glyphTextureHeight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, glyphTextureData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//	prepare vertex buffer
	glGenBuffers(1, &m_vertBuffer.m_bufferIndex);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertBuffer.m_bufferIndex);
	glBufferData(GL_ARRAY_BUFFER, m_vertBuffer.m_vertCount * GetVertSize(m_vertBuffer.GetType()), m_vertBuffer.m_pData, GL_STATIC_DRAW);	

	//	prepare vao
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vertBuffer.m_bufferIndex);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	Renderer::SetAttributes(&m_vertBuffer);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void LiveTextGlyph::Draw(const Color4 &color)
{
	glActiveTexture(GL_TEXTURE0 + MaterialData::eDiffuse);
	glBindTexture(GL_TEXTURE_2D, m_textureHandle);

	DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();

	pDefaultShaderProgram->EnableTexture(		m_vertBuffer.m_vertType & kChannel_Texcoord);
	pDefaultShaderProgram->EnableVertexColor(	m_vertBuffer.m_vertType & kChannel_Color);
	pDefaultShaderProgram->EnableLighting(		m_vertBuffer.m_vertType & kChannel_Normal);
	pDefaultShaderProgram->EnableSkinning(		m_vertBuffer.m_vertType & kChannel_CharacterAnimation_BoneIndices);

	pDefaultShaderProgram->EnableAdditionalColor(true, &color);
	
	Renderer::DrawBuffer(kQuads, &m_vertBuffer, m_vao);

	pDefaultShaderProgram->EnableAdditionalColor(false, NULL);
}