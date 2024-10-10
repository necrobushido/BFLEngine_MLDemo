#include "Tga.h"

using namespace std;

#include "TextureData.h"

namespace
{
	TGA::Header	uTGAcompare = {0,0, 2,0,0,0,0,0,0,0,0,0};	// Uncompressed TGA Header
	TGA::Header	cTGAcompare = {0,0,10,0,0,0,0,0,0,0,0,0};	// Compressed TGA Header
}

//	static
bool TGA::LoadUncompressedTGA(TextureData *texture, const TGA &tgaFile)
{
	texture->m_width = tgaFile.m_stats.m_width;
	texture->m_height = tgaFile.m_stats.m_height;
	texture->m_bpp = tgaFile.m_stats.m_bpp;

	if( texture->m_bpp == 24 )
	{
		texture->m_type	= GL_RGB;
	}
	else
	{
		texture->m_type	= GL_RGBA;
	}

	u8	bytesPerPixel = texture->m_bpp >> 3;
	u32	imageSize = texture->m_width * texture->m_height * bytesPerPixel;
	texture->m_pData = new u8[imageSize];
	const u8	*tgaData = &tgaFile.m_dataStart;
	memcpy(texture->m_pData, tgaData, imageSize);

	// Byte Swapping Optimized By Steve Thomas
	for(u32 cswap = 0; cswap < imageSize; cswap += bytesPerPixel)
	{
		texture->m_pData[cswap] ^= texture->m_pData[cswap+2] ^=
		texture->m_pData[cswap] ^= texture->m_pData[cswap+2];
	}

	if( (tgaFile.m_stats.m_imageDescriptor & (1 << 5)) != 0 )
	{
		//	would prob be better to just read it in flipped
		texture->FlipImageVertically();
	}

	return true;
}

bool TGA::LoadCompressedTGA(TextureData *texture, const TGA &tgaFile)
{ 
	texture->m_width = tgaFile.m_stats.m_width;
	texture->m_height = tgaFile.m_stats.m_height;
	texture->m_bpp = tgaFile.m_stats.m_bpp;

	if( texture->m_bpp == 24 )
	{
		texture->m_type	= GL_RGB;
	}
	else
	{
		texture->m_type	= GL_RGBA;
	}

	u8	bytesPerPixel = texture->m_bpp >> 3;
	u32	imageSize = texture->m_width * texture->m_height * (bytesPerPixel);
	texture->m_pData = new u8[imageSize];
	const u8	*tgaData = &tgaFile.m_dataStart;

	u32	pixelcount = texture->m_width * texture->m_height;
	u32	currentpixel = 0;
	u32	currentbyte = 0;
	Assert(bytesPerPixel == 3 || bytesPerPixel == 4);
	u8	colorbuffer[4];			// Storage for 1 pixel

	u32	dataPos = 0;

	memset(texture->m_pData, 0, imageSize);

	do
	{
		u8		chunkheader;
		//chunkheader = tgaData[dataPos++];
		memcpy(&chunkheader, &tgaData[dataPos], 1);
		dataPos++;
		bool	isRunLengthPacket = (chunkheader & (1 << 7)) == (1 << 7);
		u8		pixelsInPacket = (chunkheader & (~(1 << 7))) + 1;

		if( !isRunLengthPacket )
		{
			//	write "pixelsInPacket" number of pixels directly from the data to the texture
			//DebugPrintf("writing %d pixels directly\n", pixelsInPacket);
			for(u8 counter = 0; counter < pixelsInPacket; counter++)
			{
				memcpy(colorbuffer, &tgaData[dataPos], bytesPerPixel);
				dataPos += bytesPerPixel;
				
				memcpy(&texture->m_pData[currentbyte  ], &colorbuffer[2], 1);
				memcpy(&texture->m_pData[currentbyte+1], &colorbuffer[1], 1);
				memcpy(&texture->m_pData[currentbyte+2], &colorbuffer[0], 1);

				if( bytesPerPixel == 4 )
				{
					memcpy(&texture->m_pData[currentbyte+3], &colorbuffer[3], 1);
				}

				currentbyte += bytesPerPixel;
				Assert(currentbyte <= imageSize);

				currentpixel++;	
				Assert( currentpixel <= pixelcount );
			}
		}
		else
		{
			//	write "pixelsInPacket" number of pixels based on a single color
			memcpy(colorbuffer, &tgaData[dataPos], bytesPerPixel);
			dataPos += bytesPerPixel;
			
			//DebugPrintf("writing %d RLE pixels of color %d %d %d\n", pixelsInPacket, colorbuffer[2], colorbuffer[1], colorbuffer[0]);
			
			for(u8 counter = 0; counter < pixelsInPacket; counter++)
			{
				memcpy(&texture->m_pData[currentbyte  ], &colorbuffer[2], 1);
				memcpy(&texture->m_pData[currentbyte+1], &colorbuffer[1], 1);
				memcpy(&texture->m_pData[currentbyte+2], &colorbuffer[0], 1);

				if( bytesPerPixel == 4 )
				{
					memcpy(&texture->m_pData[currentbyte+3], &colorbuffer[3], 1);
				}

				currentbyte += bytesPerPixel;
				Assert(currentbyte <= imageSize);
				
				currentpixel++;
				Assert( currentpixel <= pixelcount );
			}
		}
	}
	while(currentpixel < pixelcount);

	if( (tgaFile.m_stats.m_imageDescriptor & (1 << 5)) != 0 )
	{
		//	would prob be better to just read it in flipped
		texture->FlipImageVertically();
	}

	return true;																		// return success
}

bool TGA::Load(TextureData *texture, FileRef<TGA> tgaFile)
{
	bool	returnValue = false;
	if( memcmp(&uTGAcompare, &tgaFile->m_header, sizeof(Header)) == 0 )
	{
		returnValue = LoadUncompressedTGA(texture, tgaFile);
	}
	else 
	if( memcmp(&cTGAcompare, &tgaFile->m_header, sizeof(Header)) == 0 )
	{
		returnValue = LoadCompressedTGA(texture, tgaFile);
	}
	else
	{
		//	wrong tga format
		Assert(0);
	}

	return returnValue;
}