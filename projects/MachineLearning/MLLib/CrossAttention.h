#pragma once

#include "SharedMLHeader.h"

//
class CrossAttentionImpl : public torch::nn::Module
{
public:
	CrossAttentionImpl(int numHeads, int numEmbedDimensions, int crossEmbedDimension, bool inputProjBias, bool outputProjBias, bool causalMask);

    torch::Tensor forward(torch::Tensor x, torch::Tensor y);

private:
	torch::nn::Linear		m_queryMtx;
	torch::nn::Linear		m_keyMtx;
	torch::nn::Linear		m_valueMtx;
	torch::nn::Linear		m_proj;
	int						m_numHeads;
	int						m_numEmbedDimensions;
	int						m_headEmbedSize;
};

TORCH_MODULE(CrossAttention);

//
class FlashCrossAttentionImpl : public torch::nn::Module
{
public:
	FlashCrossAttentionImpl(int numHeads, int numEmbedDimensions, int crossEmbedDimension, bool inputProjBias, bool outputProjBias, bool causalMask);

    torch::Tensor forward(torch::Tensor x, torch::Tensor y);

private:
	torch::nn::Linear		m_queryMtx;
	torch::nn::Linear		m_keyMtx;
	torch::nn::Linear		m_valueMtx;
	torch::nn::Linear		m_proj;
	int						m_numHeads;
	int						m_numEmbedDimensions;
	int						m_headEmbedSize;
};

TORCH_MODULE(FlashCrossAttention);
