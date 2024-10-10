#pragma once

#include "types.h"
#include "FileManager.h"
#include "TextureData.h"

class PNG
{
public:
	static bool Load(TextureData *texture, FileRef<PNG> pngFile);
};