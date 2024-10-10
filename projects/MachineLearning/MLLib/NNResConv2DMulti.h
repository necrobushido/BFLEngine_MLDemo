#pragma once

#include "types.h"

#include "SharedMLHeader.h"

//
class NNResConv2DMultiEncoderImpl : public torch::nn::Module 
{
public:
    NNResConv2DMultiEncoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, std::vector<int> channelMult, int numResBlocksPerEntry, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(NNResConv2DMultiEncoder);

//
class NNResConv2DMultiDecoderImpl : public torch::nn::Module 
{
public:
    NNResConv2DMultiDecoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, std::vector<int> channelMult, int numResBlocksPerEntry, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(NNResConv2DMultiDecoder);

//
class NNResConv2DMultiBlockImpl : public torch::nn::Module 
{
public:
    NNResConv2DMultiBlockImpl(int inputChannels, int outputChannels, int normSize, double dropout, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq1;
	torch::nn::Sequential	m_residualSeq;
};

TORCH_MODULE(NNResConv2DMultiBlock);

//
class NNResConv2DResizerEncoderImpl : public torch::nn::Module 
{
public:
    NNResConv2DResizerEncoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(NNResConv2DResizerEncoder);

//
class NNResConv2DResizerDecoderImpl : public torch::nn::Module 
{
public:
    NNResConv2DResizerDecoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(NNResConv2DResizerDecoder);

