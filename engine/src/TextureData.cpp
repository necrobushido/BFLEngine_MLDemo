#include "TextureData.h"

#include "FileManager.h"
#include "Tga.h"
#include "Jpg.h"
#include "PngFormat.h"

TextureData::TextureData():
	m_pData(NULL)
{
}

TextureData::~TextureData()
{
	delete [] m_pData;
}

void TextureData::FlipImageVertically()
{
	u8	*tempData = NULL;

	u32	rowLength;
	u32	loopCount;
	u32	rowIdx;

	rowLength = m_width * (m_bpp >> 3);
	loopCount = m_height / 2;

	tempData = new u8[rowLength];

	//  Flip the image
	for(u32 i = 0; i < loopCount; i++)
	{
		rowIdx = ((m_height-1)-i) * rowLength;
		
		memcpy(tempData, &m_pData[i*rowLength], rowLength);
		memcpy(&m_pData[i*rowLength], &m_pData[rowIdx], rowLength);
		memcpy(&m_pData[rowIdx], tempData, rowLength);
	}

	delete [] tempData;
}

void TextureData::FlipImageHorizontally()
{
	u8	*tempData = NULL;

	u32	rowLength;
	u32	loopCount;

	u32	pixelSize = m_bpp >> 3;
	rowLength = m_width * pixelSize;
	loopCount = m_width / 2;

	tempData = new u8[pixelSize];

	//  Flip the image
	for(u32 columnIdx = 0; columnIdx < loopCount; columnIdx++)
	{
		u32	columnOffset = columnIdx * pixelSize;
		u32	targetColumnOffset = ((m_width-1) - columnIdx) * pixelSize;
		for(u32 rowIdx = 0; rowIdx < m_height; rowIdx++)
		{
			memcpy(tempData, &m_pData[rowIdx*rowLength+columnOffset], pixelSize);
			memcpy(&m_pData[rowIdx*rowLength+columnOffset], &m_pData[rowIdx*rowLength+targetColumnOffset], pixelSize);
			memcpy(&m_pData[rowIdx*rowLength+targetColumnOffset], tempData, pixelSize);
		}
	}

	delete [] tempData;
}

void TextureData::Load(const char* filename)
{
	char	extensionBuffer[256];
	GetFileExtension(filename, extensionBuffer, 256);

	if( strcmp(extensionBuffer, "tga") == 0 )
	{
		FileRef<TGA>	fileRef = g_fileManager->MakeRef(filename);
		TGA::Load(this, fileRef);
	}
	else
	if( strcmp(extensionBuffer, "jpg") == 0 )
	{
		//	linker error that I didn't care to fix atm
		{
			FileRef<TGA>	fileRef = g_fileManager->MakeRef("Blank.tga");
			TGA::Load(this, fileRef);
		}
		//FileRef<JPG>	fileRef = g_fileManager->MakeRef(filename);
		//JPG::Load(this, fileRef);
	}
	else
	if( strcmp(extensionBuffer, "png") == 0 )
	{
		//	linker error that I didn't care to fix atm
		{
			FileRef<TGA>	fileRef = g_fileManager->MakeRef("Blank.tga");
			TGA::Load(this, fileRef);
		}
		//FileRef<PNG>	fileRef = g_fileManager->MakeRef(filename);
		//PNG::Load(this, fileRef);
	}
	else
	{
		AssertMsg(false, "Unrecognized texture extension \"%s\"\n", extensionBuffer);
	}
}