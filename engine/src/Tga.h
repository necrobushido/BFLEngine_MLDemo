#pragma once

#include "types.h"
#include "FileManager.h"
#include "TextureData.h"

class TGA
{
public:
	static bool Load(TextureData *texture, FileRef<TGA> tgaFile);

private:
	static bool LoadUncompressedTGA(TextureData *texture, const TGA &tgaFile);
	static bool LoadCompressedTGA(TextureData *texture, const TGA &tgaFile);

public:
	struct Header
	{
		u8		header[12];
	};

	struct Stats
	{
		u16		m_width;
		u16		m_height;
		u8		m_bpp;
		u8		m_imageDescriptor;	//	bits 0-3 for alpha channel depth, bit 4 for x direction, bit 5 for y direction
	};

public:
	Header	m_header;
	Stats	m_stats;
	u8		m_dataStart;	//	first element of the data array, which extends past the class definitino
};