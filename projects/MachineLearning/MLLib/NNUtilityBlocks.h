#pragma once

#include "SharedMLHeader.h"

//
class NNUpsampleConvBlockImpl : public torch::nn::Module
{
public:
	NNUpsampleConvBlockImpl(int inputChannels);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq1;
};

TORCH_MODULE(NNUpsampleConvBlock);

//
class NNShrinkConvBlockImpl : public torch::nn::Module
{
public:
	NNShrinkConvBlockImpl(int inputChannels);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq1;
};

TORCH_MODULE(NNShrinkConvBlock);

//
class NNUpsampleConv2DBlockImpl : public torch::nn::Module
{
public:
	NNUpsampleConv2DBlockImpl(int inputChannels);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq1;
};

TORCH_MODULE(NNUpsampleConv2DBlock);

//
class NNShrinkConv2DBlockImpl : public torch::nn::Module
{
public:
	NNShrinkConv2DBlockImpl(int inputChannels);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq1;
};

TORCH_MODULE(NNShrinkConv2DBlock);

//
class NNResidualBlock1DImpl : public torch::nn::Module
{
public:
	NNResidualBlock1DImpl(int inputChannels, int outputChannels, int normGroups, double dropout, eNonLinearity nonLinearity);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq1;
	torch::nn::Sequential	m_residualSeq;
};

TORCH_MODULE(NNResidualBlock1D);

//
class NNResidualBlock2DImpl : public torch::nn::Module 
{
public:
    NNResidualBlock2DImpl(int inputChannels, int outputChannels, int normGroups, double dropout, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq1;
	torch::nn::Sequential	m_residualSeq;
};

TORCH_MODULE(NNResidualBlock2D);

//
class NNDirectConvBlockImpl : public torch::nn::Module
{
public:
	NNDirectConvBlockImpl(int inputChannels, int outputChannels);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq1;
	int						m_outputChannels;
};

TORCH_MODULE(NNDirectConvBlock);

//
class PermuteLayerImpl : public torch::nn::Module
{
public:
	PermuteLayerImpl(int batch, int channels, int seq, int embed):
		m_batch(batch),
		m_channels(channels),
		m_seq(seq),
		m_embed(embed)
	{
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = x.permute({m_batch, m_channels, m_seq, m_embed});
		x = x.contiguous();

		return x;
	}

private:
	int	m_batch;
	int	m_channels;
	int	m_seq;
	int	m_embed;
};

TORCH_MODULE(PermuteLayer);

//
class Transpose12LayerImpl : public torch::nn::Module
{
public:
	Transpose12LayerImpl()
	{
	}

    torch::Tensor forward(torch::Tensor x)
	{
		return x.transpose(1, 2).contiguous();
	}
};

TORCH_MODULE(Transpose12Layer);

//
class Transpose13LayerImpl : public torch::nn::Module
{
public:
	Transpose13LayerImpl()
	{
	}

    torch::Tensor forward(torch::Tensor x)
	{
		return x.transpose(1, 3).contiguous();
	}
};

TORCH_MODULE(Transpose13Layer);

//
class Transpose23LayerImpl : public torch::nn::Module
{
public:
	Transpose23LayerImpl()
	{
	}

    torch::Tensor forward(torch::Tensor x)
	{
		return x.transpose(2, 3).contiguous();
	}
};

TORCH_MODULE(Transpose23Layer);

//
class ChannelSqueezeLayerImpl : public torch::nn::Module
{
public:
	ChannelSqueezeLayerImpl()
	{
	}

    torch::Tensor forward(torch::Tensor x)
	{
		return x.squeeze(1);
	}
};

TORCH_MODULE(ChannelSqueezeLayer);

//
class ValueSqueezeLayerImpl : public torch::nn::Module
{
public:
	ValueSqueezeLayerImpl()
	{
	}

    torch::Tensor forward(torch::Tensor x)
	{
		return x.squeeze(-1);
	}
};

TORCH_MODULE(ValueSqueezeLayer);

//
class InterpolateBlockImpl : public torch::nn::Module 
{
public:
	InterpolateBlockImpl(double xScale, double yScale):
		m_interpX(xScale),
		m_interpY(yScale)
	{
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = torch::nn::functional::interpolate(x, torch::nn::functional::InterpolateFuncOptions().scale_factor(std::vector<double>({m_interpX, m_interpY})).mode(torch::kNearest));

		return x;
	}

	double	m_interpX;
	double	m_interpY;
};

TORCH_MODULE(InterpolateBlock);

//
class Concat23LayerImpl : public torch::nn::Module
{
public:
	Concat23LayerImpl();

    torch::Tensor forward(torch::Tensor x);
};

TORCH_MODULE(Concat23Layer);

//
class NNEncoderAttentionBlockImpl : public torch::nn::Module
{
public:
	NNEncoderAttentionBlockImpl(int channels, double dropout);

    torch::Tensor forward(torch::Tensor x);

public:
	torch::nn::Sequential	m_seq;

};

TORCH_MODULE(NNEncoderAttentionBlock);
