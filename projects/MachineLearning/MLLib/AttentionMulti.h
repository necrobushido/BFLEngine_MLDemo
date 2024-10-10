#pragma once

//#include "types.h"

#include "SharedMLHeader.h"

class AttentionMultiImpl : public torch::nn::Module 
{
public:
	//	this version does not have input and output projections
	AttentionMultiImpl(int numHeads, int hiddenLayerSize, int hiddenLayerCount, bool causal, double dropout, eNonLinearity nonLinearity = kNL_Relu);

	//	this version has input and output projections
    AttentionMultiImpl(int inputSize, int outputSize, int numHeads, int hiddenLayerSize, int hiddenLayerCount, bool causal, double dropout, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(AttentionMulti);

//
class AttentionMultiBlockImpl : public torch::nn::Module 
{
public:
    AttentionMultiBlockImpl(int numHeads, int embedSize, double dropout, bool causal, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_attentionSeq;
	torch::nn::Sequential	m_nlSeq;
};

TORCH_MODULE(AttentionMultiBlock);

