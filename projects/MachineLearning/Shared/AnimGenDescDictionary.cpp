#include "AnimGenDescDictionary.h"

#include "FileManager.h"

namespace
{
	const std::string	kNullTagName = "";

	const char*			kSavedDictName = "AnimGenDescDictionary.dat";
}

//
AnimGenDescDictionary2*	g_animGenDescDictionary2 = nullptr;

AnimGenDescDictionary2::AnimGenDescDictionary2()
{
	Assert(g_animGenDescDictionary2 == nullptr);
	g_animGenDescDictionary2 = this;
}

AnimGenDescDictionary2::~AnimGenDescDictionary2()
{
	Assert(g_animGenDescDictionary2 == this);
	g_animGenDescDictionary2 = nullptr;
}

int AnimGenDescDictionary2::GetVocabSize()
{
	return 256;	//	ASCII size right now; maybe expand to an actual tokenization scheme later
}

int AnimGenDescDictionary2::Tokenize(const std::string& inputString, int* outputTokens, int maxTokens, bool doPadding)
{
	//	for now just copy the character values over as ASCII codes
	int		currentOutputTokenIdx = 0;
	size_t	inputStringSize = inputString.size();
	for(size_t inputStringIdx = 0; inputStringIdx < inputStringSize && currentOutputTokenIdx < maxTokens; inputStringIdx++)
	{
		outputTokens[currentOutputTokenIdx] = inputString[inputStringIdx];

		++currentOutputTokenIdx;
	}

	if( doPadding )
	{
		while(currentOutputTokenIdx < maxTokens)
		{		
			outputTokens[currentOutputTokenIdx] = 0;

			++currentOutputTokenIdx;
		}
	}

	return currentOutputTokenIdx;
}