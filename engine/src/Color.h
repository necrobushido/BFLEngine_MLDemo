#pragma once

#include "dataTypes.h"

struct Color3
{
	Color3(){}

	Color3(float _r, float _g, float _b)
	{
		r = _r;
		g = _g;
		b = _b;
	}

	float	r;
	float	g;
	float	b;

	//
	static const Color3	BLACK;
	static const Color3	WHITE;
	static const Color3	RED;
	static const Color3	GREEN;
	static const Color3	BLUE;
};

struct Color4 : public Color3
{
	Color4(){}

	Color4(float _r, float _g, float _b, float _a):
		Color3(_r, _g, _b)
	{
		a = _a;
	}

	void SetFromString(const char* string);
	void WriteToString(char* outputBuffer);

	float	a;

	//
	static const Color4	BLACK;
	static const Color4	WHITE;
	static const Color4	RED;
	static const Color4	GREEN;
	static const Color4	BLUE;
};