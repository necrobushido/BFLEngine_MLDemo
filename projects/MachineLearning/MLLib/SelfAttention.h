#pragma once

#include "SharedMLHeader.h"

//
class SelfAttentionImpl : public torch::nn::Module
{
public:
	SelfAttentionImpl(int numHeads, int numEmbedDimensions, bool inputProjBias, bool outputProjBias, bool causalMask, double weightsDropout = 0.0);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Linear		m_attentionComponentWeights;
	torch::nn::Linear		m_proj;
	torch::nn::Dropout		m_weightsDropout;
	int						m_numHeads;
	int						m_numEmbedDimensions;
	int						m_headEmbedSize;
	bool					m_causalMask;
};

TORCH_MODULE(SelfAttention);

//
class FlashSelfAttentionImpl : public torch::nn::Module
{
public:
	FlashSelfAttentionImpl(int numHeads, int numEmbedDimensions, bool inputProjBias, bool outputProjBias, bool causalMask, double weightsDropout = 0.0);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Linear		m_attentionComponentWeights;
	torch::nn::Linear		m_proj;
	double					m_weightsDropout;
	int						m_numHeads;
	int						m_numEmbedDimensions;
	int						m_headEmbedSize;
	bool					m_causalMask;
};

TORCH_MODULE(FlashSelfAttention);
