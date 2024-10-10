#pragma once

#include "SharedMLHeader.h"

//
class NNCLIPProjHeadImpl : public torch::nn::Module 
{
public:
	NNCLIPProjHeadImpl(int inputSize, double dropout);
	virtual ~NNCLIPProjHeadImpl();

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_projSeq;
	torch::nn::Sequential	m_nlSeq;
};

TORCH_MODULE(NNCLIPProjHead);

//
class NNCLIPImpl : public torch::nn::Module 
{
public:
	enum
	{
		kMaxTokens = 64,
		kTextEmbedSize = 1024,
		kAnimEmbedSize = 4096,
		kSharedEmbedSize = kAnimEmbedSize
	};

public:
	NNCLIPImpl();
	virtual ~NNCLIPImpl();

    torch::Tensor forward(torch::Tensor inputTextTokenized);

	torch::Tensor EncodeText(torch::Tensor inputTextTokenized);
	torch::Tensor EncodeAnim(torch::Tensor inputAnimLatent);

	torch::Tensor ProjectTextToShared(torch::Tensor text);
	torch::Tensor ProjectAnimToShared(torch::Tensor anim);

	torch::Tensor GetLNLogitScale(){ return m_lnLogitScale; }

	void GetGainOrBiasParams(std::vector<torch::Tensor>* gainOrBiasParamsOut, std::vector<torch::Tensor>* nonGainOrBiasParamsOut);

private:
	torch::nn::Embedding	m_tokenEmbedding;
	torch::Tensor			m_positionEmbedding;
	torch::nn::Sequential	m_textTransformerSeq;
	torch::nn::Sequential	m_animEncoderSeq;
	torch::Tensor			m_lnLogitScale;
	//NNCLIPProjHead			m_textToSharedProj;
	//NNCLIPProjHead			m_animToSharedProj;

	torch::Tensor			m_textProjection;
};

TORCH_MODULE(NNCLIP);

extern NNCLIPImpl* g_NNCLIP;