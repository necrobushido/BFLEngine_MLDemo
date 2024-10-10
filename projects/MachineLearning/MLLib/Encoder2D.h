#pragma once

#include "types.h"

#include "SharedMLHeader.h"

//
class Encoder2DImpl : public torch::nn::Module 
{
public:
    Encoder2DImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int normGroups, double dropout, std::vector<int> channelMult, int numResBlocksPerEntry, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(Encoder2D);

//
class Decoder2DImpl : public torch::nn::Module 
{
public:
    Decoder2DImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int normGroups, double dropout, std::vector<int> channelMult, int numResBlocksPerEntry, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(Decoder2D);