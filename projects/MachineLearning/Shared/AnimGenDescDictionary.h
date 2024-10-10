#pragma once

#include "types.h"

//
class AnimGenDescDictionary2
{
public:
	AnimGenDescDictionary2();
	~AnimGenDescDictionary2();

	int GetVocabSize();

	int Tokenize(const std::string& inputString, int* outputTokens, int maxTokens, bool doPadding);
};

extern AnimGenDescDictionary2* g_animGenDescDictionary2;
