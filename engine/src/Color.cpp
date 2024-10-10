#include "Color.h"

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

const Color3	Color3::BLACK(	0.0f, 0.0f, 0.0f);
const Color3	Color3::WHITE(	1.0f, 1.0f, 1.0f);
const Color3	Color3::RED(	1.0f, 0.0f, 0.0f);
const Color3	Color3::GREEN(	0.0f, 1.0f, 0.0f);
const Color3	Color3::BLUE(	0.0f, 0.0f, 1.0f);

const Color4	Color4::BLACK(	0.0f, 0.0f, 0.0f, 1.0f);
const Color4	Color4::WHITE(	1.0f, 1.0f, 1.0f, 1.0f);
const Color4	Color4::RED(	1.0f, 0.0f, 0.0f, 1.0f);
const Color4	Color4::GREEN(	0.0f, 1.0f, 0.0f, 1.0f);
const Color4	Color4::BLUE(	0.0f, 0.0f, 1.0f, 1.0f);

void Color4::SetFromString(const char* string)
{
	const char*	firstNumberStart = string;
	const char*	firstComma = strchr(firstNumberStart, ',');
	const char*	secondNumberStart = &firstComma[1];
	const char*	secondComma = strchr(secondNumberStart, ',');
	const char*	thirdNumberStart = &secondComma[1];
	const char*	thirdComma = strchr(thirdNumberStart, ',');
	const char*	fourthNumberStart = &thirdComma[1];

	char		number[256];
	u64			numCharsInFloat;
	numCharsInFloat = firstComma - firstNumberStart;
	strncpy(number, firstNumberStart, numCharsInFloat);
	number[numCharsInFloat] = '\0';
	r = (f32)atof(number);

	numCharsInFloat = secondComma - secondNumberStart;
	strncpy(number, secondNumberStart, numCharsInFloat);
	number[numCharsInFloat] = '\0';
	g = (f32)atof(number);

	numCharsInFloat = thirdComma - thirdNumberStart;
	strncpy(number, thirdNumberStart, numCharsInFloat);
	number[numCharsInFloat] = '\0';
	b = (f32)atof(number);

	strcpy(number, fourthNumberStart);
	a = (f32)atof(number);
}

void Color4::WriteToString(char* outputBuffer)
{
	sprintf(outputBuffer, "%f, %f, %f, %f", r, g, b, a);
}