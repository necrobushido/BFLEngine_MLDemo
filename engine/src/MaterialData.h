#pragma once

#include "Color.h"

#ifndef BFL_TOOLS
#include "Texture.h"
#endif

class MaterialData
{
public:
	enum
	{
		eEmissive,
		eAmbient,
		eDiffuse,
		eSpecular,

		kMaterialChannelCount
	};

	class MaterialChannel
	{
	public:
		MaterialChannel(){}
		~MaterialChannel(){}

	public:
		Color4		color;
		char		texture[256];
#ifdef BFL_TOOLS
		//	don't want the tools to depend on Texture.h
		int*		textureRefPad;
#else
		//	but we might want to use the tool code in the engine proper, so keep textureRefPad around
		union
		{
			TextureRef	textureRef;
			int*		textureRefPad;
		};
#endif
	};

public:
	MaterialChannel	channels[kMaterialChannelCount];
	float			specularShininess;
};