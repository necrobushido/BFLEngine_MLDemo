#pragma once

#include "AsciiTokenizer.h"
#include "SharedMLHeader.h"

#include "AsciiWordDelimitedTokenizer.h"

//
class NNTokenizer
{
public:
	NNTokenizer();
	virtual ~NNTokenizer();

	void Init();

	void Encode(const std::string& inputString, std::vector<int>* tokensOut);
	void Decode(const std::vector<int>& inputTokens, std::string* stringOut) const;

	torch::Tensor EncodePaddedTensor(const std::string& inputString, int maxTokens, bool padToMax, bool frontPadSpace);
	void DecodeTensor(torch::Tensor input, std::string* output) const;

	int GetStartToken() const;
	int GetEndToken() const;

	int GetVocabSize() const;

protected:
	//AsciiTokenizer	m_baseTokenizer;

	AsciiWordDelimitedTokenizer	m_baseTokenizer;
};

extern NNTokenizer* g_NNTokenizer;