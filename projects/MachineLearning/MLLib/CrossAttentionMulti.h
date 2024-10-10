#pragma once

//#include "types.h"

#include "SharedMLHeader.h"
#include "CrossAttention.h"

class CrossAttentionMultiImpl : public torch::nn::Module 
{
public:
	CrossAttentionMultiImpl(int numHeads, int embedSize, int crossEmbedSize, int hiddenLayerCount, bool causal, double dropout, eNonLinearity nonLinearity = kNL_Relu);
    CrossAttentionMultiImpl(int inputSize, int outputSize, int numHeads, int embedSize, int crossEmbedSize, int hiddenLayerCount, bool causal, double dropout, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x, torch::Tensor crossValue);

private:
	torch::nn::Sequential	m_inputSeq;
	torch::nn::Sequential	m_outputSeq;
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(CrossAttentionMulti);

//
class CrossAttentionMultiBlockImpl : public torch::nn::Module 
{
public:
    CrossAttentionMultiBlockImpl(int numHeads, int embedSize, int crossEmbedSize, double dropout, bool causal, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x, torch::Tensor crossValue);

private:
	torch::nn::Sequential	m_attentionSeq;
	torch::nn::Sequential	m_nlSeq;
	torch::nn::LayerNorm	m_crossAttentionLayerNorm;
	CrossAttention			m_crossAttention;
	torch::nn::Dropout		m_crossDropout;
};

TORCH_MODULE(CrossAttentionMultiBlock);

