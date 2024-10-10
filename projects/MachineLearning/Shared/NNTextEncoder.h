#pragma once

#include "SharedMLHeader.h"

//
class NNTextEncoderImpl : public torch::nn::Module 
{
public:
	enum
	{
		kMaxTokens = 128,
		kEmbedSize = 768
	};

public:
	NNTextEncoderImpl();
	virtual ~NNTextEncoderImpl();

    torch::Tensor forward(torch::Tensor inputTextTokenized);

	//	keeping this separate so that I can use the encoded output without mapping back to characters
	torch::Tensor LanguageModelingForward(torch::Tensor inputEncodedText);

private:
	torch::nn::Embedding	m_tokenEmbedding;
	torch::Tensor			m_positionEmbedding;
	torch::nn::Sequential	m_seq;
	torch::nn::Linear		m_languageModelingHead;
};

TORCH_MODULE(NNTextEncoder);

extern NNTextEncoderImpl* g_NNTextEncoder;