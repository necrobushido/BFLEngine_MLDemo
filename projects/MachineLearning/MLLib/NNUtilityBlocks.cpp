#include "NNUtilityBlocks.h"

#include "SelfAttention.h"

#include "types.h"

//
NNUpsampleConvBlockImpl::NNUpsampleConvBlockImpl(int inputChannels):
	m_seq1(register_module("m_seq1", torch::nn::Sequential()))
{
	m_seq1->push_back("upsample", torch::nn::Upsample(torch::nn::UpsampleOptions().scale_factor(std::vector<double>({2.0}))));
	m_seq1->push_back("upsampleConv", torch::nn::Conv1d(torch::nn::Conv1dOptions(inputChannels, inputChannels, 3).padding(1)));
}

torch::Tensor NNUpsampleConvBlockImpl::forward(torch::Tensor x)
{
	x = m_seq1->forward(x);

	return x;
}

//
NNShrinkConvBlockImpl::NNShrinkConvBlockImpl(int inputChannels):
	m_seq1(register_module("m_seq1", torch::nn::Sequential()))
{
	m_seq1->push_back("conv1d_1", torch::nn::Conv1d(torch::nn::Conv1dOptions(inputChannels, inputChannels, 3).stride(2).padding(0)));
}

torch::Tensor NNShrinkConvBlockImpl::forward(torch::Tensor x)
{
	x = torch::nn::functional::pad(x, torch::nn::functional::PadFuncOptions({0, 1}));
	x = m_seq1->forward(x);

	return x;
}

//
NNUpsampleConv2DBlockImpl::NNUpsampleConv2DBlockImpl(int inputChannels):
	m_seq1(register_module("m_seq1", torch::nn::Sequential()))
{
	m_seq1->push_back("upsample", torch::nn::Upsample(torch::nn::UpsampleOptions().scale_factor(std::vector<double>({1.0, 2.0}))));
	m_seq1->push_back("upsampleConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, inputChannels, 3).padding(1)));
}

torch::Tensor NNUpsampleConv2DBlockImpl::forward(torch::Tensor x)
{
	x = m_seq1->forward(x);

	return x;
}

//
NNShrinkConv2DBlockImpl::NNShrinkConv2DBlockImpl(int inputChannels):
	m_seq1(register_module("m_seq1", torch::nn::Sequential()))
{
	m_seq1->push_back("conv1d_1", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, inputChannels, 3).stride({1, 2}).padding(0)));
}

torch::Tensor NNShrinkConv2DBlockImpl::forward(torch::Tensor x)
{
	x = torch::nn::functional::pad(x, torch::nn::functional::PadFuncOptions({0, 1, 1, 1}));
	x = m_seq1->forward(x);

	return x;
}

//
NNResidualBlock1DImpl::NNResidualBlock1DImpl(int inputChannels, int outputChannels, int normGroups, double dropout, eNonLinearity nonLinearity):
	m_seq1(register_module("m_seq1", torch::nn::Sequential())),
	m_residualSeq(register_module("m_residualSeq", torch::nn::Sequential()))
{
	m_seq1->push_back(register_module("groupNorm1", torch::nn::GroupNorm(normGroups, inputChannels)));
	AddNonLinearityModule(m_seq1, nonLinearity, "NL1");
	m_seq1->push_back(register_module("conv1", torch::nn::Conv1d(torch::nn::Conv1dOptions(inputChannels, outputChannels, 3).bias(false).padding(1))));	//	directly followed by a norm so shouldn't need a bias?

	m_seq1->push_back(register_module("groupNorm2", torch::nn::GroupNorm(normGroups, outputChannels)));
	AddNonLinearityModule(m_seq1, nonLinearity, "NL2");
	m_seq1->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));	//	the SD repository had it here
	m_seq1->push_back(register_module("conv2", torch::nn::Conv1d(torch::nn::Conv1dOptions(outputChannels, outputChannels, 3).padding(1))));
	//m_seq1->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));	//	I would think this would go here

	if( inputChannels == outputChannels )
	{
		m_residualSeq->push_back(register_module("residualLayer", torch::nn::Identity()));
	}
	else
	{
		m_residualSeq->push_back(register_module("residualLayer", torch::nn::Conv1d(torch::nn::Conv1dOptions(inputChannels, outputChannels, 1).padding(0))));
	}
}

torch::Tensor NNResidualBlock1DImpl::forward(torch::Tensor x)
{
	x = m_seq1->forward(x) + m_residualSeq->forward(x);

	return x;
}

//
NNResidualBlock2DImpl::NNResidualBlock2DImpl(int inputChannels, int outputChannels, int normGroups, double dropout, eNonLinearity nonLinearity):
	m_seq1(register_module("m_seq1", torch::nn::Sequential())),
	m_residualSeq(register_module("m_residualSeq", torch::nn::Sequential()))
{
	m_seq1->push_back(register_module("groupNorm1", torch::nn::GroupNorm(normGroups, inputChannels)));
	AddNonLinearityModule(m_seq1, nonLinearity, "NL1");
	m_seq1->push_back(register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, outputChannels, 3).bias(false).padding(1))));	//	directly followed by a norm so shouldn't need a bias?

	m_seq1->push_back(register_module("groupNorm2", torch::nn::GroupNorm(normGroups, outputChannels)));
	AddNonLinearityModule(m_seq1, nonLinearity, "NL2");
	m_seq1->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));	//	the SD repository had it here
	m_seq1->push_back(register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(outputChannels, outputChannels, 3).padding(1))));
	//m_seq1->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));	//	I would think this would go here

	if( inputChannels == outputChannels )
	{
		m_residualSeq->push_back(register_module("residualLayer", torch::nn::Identity()));
	}
	else
	{
		m_residualSeq->push_back(register_module("residualLayer", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, outputChannels, 1).padding(0))));
	}
}

torch::Tensor NNResidualBlock2DImpl::forward(torch::Tensor x)
{
	x = m_seq1->forward(x) + m_residualSeq->forward(x);

	return x;
}

//
NNDirectConvBlockImpl::NNDirectConvBlockImpl(int inputChannels, int outputChannels):
	m_seq1(register_module("m_seq1", torch::nn::Sequential())),
	m_outputChannels(outputChannels)
{
	m_seq1->push_back("conv1d_1", torch::nn::Conv1d(torch::nn::Conv1dOptions(inputChannels, outputChannels, 1).padding(0)));
}

torch::Tensor NNDirectConvBlockImpl::forward(torch::Tensor x)
{
	//	x should have been permuted to {batch, keyframes, bones, channels} on input here

	torch::IntArrayRef  xSizes = x.sizes();
	s64					xBatch = xSizes[0];
	s64					xKeyframes = xSizes[1];
	s64					xBones = xSizes[2];
	s64					xRotValues = xSizes[3];

	x = x.view({xBatch, xKeyframes, xBones*xRotValues});

	x = m_seq1->forward(x);

	x = x.view({xBatch, m_outputChannels, xBones, xRotValues});

	return x;
}

//
Concat23LayerImpl::Concat23LayerImpl()
{
}

torch::Tensor Concat23LayerImpl::forward(torch::Tensor x)
{
	Assert(x.sizes().size() == 4);
	return x.view({x.size(0), x.size(1), x.size(2)*x.size(3)});
}

//
namespace
{
	enum
	{
		kAttentionHeads = 1
	};

	const bool	kAttentionCausal = false;
}

NNEncoderAttentionBlockImpl::NNEncoderAttentionBlockImpl(int channels, double dropout):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	m_seq->push_back(register_module("transpose1", Transpose12Layer()));
	m_seq->push_back(register_module("attentionLayerNorm", torch::nn::LayerNorm(torch::nn::LayerNormOptions({channels}))));
	m_seq->push_back(register_module("selfAttention", SelfAttention(kAttentionHeads, channels, false, false, kAttentionCausal, dropout)));
	m_seq->push_back(register_module("transpose2", Transpose12Layer()));
	m_seq->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));
}

torch::Tensor NNEncoderAttentionBlockImpl::forward(torch::Tensor x)
{
	torch::Tensor	residue = x;

	x = x.view({residue.size(0), residue.size(1), residue.size(2) * residue.size(3)});
	x = m_seq->forward(x);
	x = x.view({residue.size(0), residue.size(1), residue.size(2), residue.size(3)});

	return x + residue;
}