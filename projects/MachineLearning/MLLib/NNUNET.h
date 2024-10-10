#pragma once

#include "SharedMLHeader.h"

//
class NNUNETTimePositioningImpl : public torch::nn::Module
{
public:
	NNUNETTimePositioningImpl(int size);

    torch::Tensor forward(torch::Tensor trainingTimestepIndices);

	torch::Tensor	m_freqs;
	int				m_size;
};

TORCH_MODULE(NNUNETTimePositioning);

//
class NNUNETTimeEmbeddingImpl : public torch::nn::Module
{
public:
	NNUNETTimeEmbeddingImpl(int timeSize, int timeEmbedSize, eNonLinearity nonLinearity = kNL_Silu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(NNUNETTimeEmbedding);

//
class NNUNETImpl : public torch::nn::Module 
{
public:
	NNUNETImpl(int inputChannels, int hiddenChannels, int numHeads, int textCrossSize, int timeSize, int normSize, double dropout, std::vector<int> channelMult, int numResBlocksPerEntry, std::vector<int> attentionResolutions, eNonLinearity nonLinearity = kNL_Silu);

	torch::Tensor forward(torch::Tensor x, torch::Tensor context, torch::Tensor time);

private:
	torch::nn::Sequential	m_outputSeq;
	
	torch::nn::Sequential	m_encoderSeq;
	torch::nn::Sequential	m_centerSeq;
	torch::nn::Sequential	m_decoderSeq;
};

TORCH_MODULE(NNUNET);