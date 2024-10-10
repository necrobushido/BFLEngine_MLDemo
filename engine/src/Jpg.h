#pragma once

#include "types.h"
#include "FileManager.h"
#include "TextureData.h"
#include "jpeglib.h"

class JPG
{
public:
	static bool Load(TextureData *texture, FileRef<JPG> jpgFile);

private:
	static void Decompress(jpeg_decompress_struct *cInfo, TextureData *texture);
};