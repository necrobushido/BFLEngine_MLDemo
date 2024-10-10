#include "Encoder2D.h"

#include "NNUtilityBlocks.h"
#include "SelfAttention.h"

//
class EncoderBlockImpl : public torch::nn::Module 
{
public:
	EncoderBlockImpl(int inChannels, int outChannels, int numResBlocks, bool doDownSample, int normGroups, double dropout, eNonLinearity nonLinearity):
		m_blockSeq(register_module("m_blockSeq", torch::nn::Sequential())),
		m_downsampleSeq(register_module("m_downsampleSeq", torch::nn::Sequential())),
		m_doDownSample(doDownSample)
	{
		int	blockIn = inChannels;
		int	blockOut = outChannels;
		for(int iBlock = 0; iBlock < numResBlocks; ++iBlock)
		{
			char	nameBuffer[256];
			sprintf(nameBuffer, "rb_%d", iBlock);
			m_blockSeq->push_back(register_module(nameBuffer, NNResidualBlock2D(blockIn, blockOut, normGroups, dropout, nonLinearity)));

			blockIn = blockOut;
		}

		if( m_doDownSample )
		{
			m_downsampleSeq->push_back(register_module("downsampleConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(blockIn, blockIn, 3).stride(2).padding(0))));
		}
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = m_blockSeq->forward(x);

		if( m_doDownSample )
		{
			x = torch::nn::functional::pad(x, torch::nn::functional::PadFuncOptions({0, 1, 0, 1}));
			x = m_downsampleSeq->forward(x);
		}

		return x;
	}

	torch::nn::Sequential	m_blockSeq;
	torch::nn::Sequential	m_downsampleSeq;
	bool					m_doDownSample;
};

TORCH_MODULE(EncoderBlock);

//
class DecoderBlockImpl : public torch::nn::Module 
{
public:
	DecoderBlockImpl(int inChannels, int outChannels, int numResBlocks, bool doUpSample, int normGroups, double dropout, eNonLinearity nonLinearity):
		m_blockSeq(register_module("m_blockSeq", torch::nn::Sequential())),
		m_upsampleSeq(register_module("m_upsampleSeq", torch::nn::Sequential())),
		m_doUpSample(doUpSample)
	{
		int	blockIn = inChannels;
		int	blockOut = outChannels;
		for(int iBlock = 0; iBlock < numResBlocks+1; ++iBlock)	//	note that the decoder block has 1 more resblock than the encoder
		{
			char	nameBuffer[256];
			sprintf(nameBuffer, "rb_%d", iBlock);
			m_blockSeq->push_back(register_module(nameBuffer, NNResidualBlock2D(blockIn, blockOut, normGroups, dropout, nonLinearity)));

			blockIn = blockOut;
		}

		if( m_doUpSample )
		{
			m_upsampleSeq->push_back(register_module("upsampleConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(blockIn, blockIn, 3).padding(1))));
		}
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = m_blockSeq->forward(x);

		if( m_doUpSample )
		{
			x = torch::nn::functional::interpolate(x, torch::nn::functional::InterpolateFuncOptions().scale_factor(std::vector<double>({2, 2})).mode(torch::kNearest));
			x = m_upsampleSeq->forward(x);
		}

		return x;
	}

	torch::nn::Sequential	m_blockSeq;
	torch::nn::Sequential	m_upsampleSeq;
	bool					m_doUpSample;
};

TORCH_MODULE(DecoderBlock);

//
Encoder2DImpl::Encoder2DImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int normGroups, double dropout, std::vector<int> channelMult, int numResBlocksPerEntry, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	int	numResolutions = (int)channelMult.size();
	Assert(numResolutions > 0);

	int	blockIn = hiddenLayerChannels * channelMult[0];
	int	blockOut = 0;

	//	channels up
	m_seq->push_back(register_module("convInput", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, blockIn, 3).padding(1))));

	//	downsample
	for(int iLevel = 0; iLevel < numResolutions; ++iLevel)
	{
		blockOut = hiddenLayerChannels * channelMult[iLevel];

		bool	doDownSample = iLevel < numResolutions - 1;

		char	nameBuffer[256];
		sprintf(nameBuffer, "entry_%d", iLevel);
		m_seq->push_back(register_module(nameBuffer, EncoderBlock(blockIn, blockOut, numResBlocksPerEntry, doDownSample, normGroups, dropout, nonLinearity)));

		blockIn = blockOut;
	}

	//	mid
	m_seq->push_back(register_module("rbMid1", NNResidualBlock2D(blockIn, blockIn, normGroups, dropout, nonLinearity)));
	m_seq->push_back(register_module("attentionMid", NNEncoderAttentionBlock(blockIn, dropout)));
	m_seq->push_back(register_module("rbMid2", NNResidualBlock2D(blockIn, blockIn, normGroups, dropout, nonLinearity)));

	//	NL + channels down
	m_seq->push_back(register_module("groupNormFinal", torch::nn::GroupNorm(normGroups, blockIn)));
	AddNonLinearityModule(m_seq, nonLinearity, "NLFinal");
	m_seq->push_back(register_module("convFinal1", torch::nn::Conv2d(torch::nn::Conv2dOptions(blockIn, outputChannels, 3).padding(1))));

	//	quant
	m_seq->push_back(register_module("quantConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(outputChannels, outputChannels, 1).padding(0))));
}

torch::Tensor Encoder2DImpl::forward(torch::Tensor x)
{
	return m_seq->forward(x);
}

//
Decoder2DImpl::Decoder2DImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int normGroups, double dropout, std::vector<int> channelMult, int numResBlocksPerEntry, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	int	numResolutions = (int)channelMult.size();
	Assert(numResolutions > 0);

	int	blockIn = hiddenLayerChannels * channelMult[numResolutions-1];
	int	blockOut = 0;

	//	quant
	m_seq->push_back(register_module("postQuantConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, inputChannels, 1).padding(0))));

	//	channels up
	m_seq->push_back(register_module("convInput2", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, blockIn, 3).padding(1))));

	//	mid
	m_seq->push_back(register_module("rbMid2", NNResidualBlock2D(blockIn, blockIn, normGroups, dropout, nonLinearity)));
	m_seq->push_back(register_module("attentionMid", NNEncoderAttentionBlock(blockIn, dropout)));
	m_seq->push_back(register_module("rbMid1", NNResidualBlock2D(blockIn, blockIn, normGroups, dropout, nonLinearity)));

	//	upsample
	for(int iLevel = numResolutions - 1; iLevel >= 0; --iLevel)
	{
		blockOut = hiddenLayerChannels * channelMult[iLevel];

		bool	doUpSample = iLevel > 0;

		char	nameBuffer[256];
		sprintf(nameBuffer, "entry_%d", iLevel);
		m_seq->push_back(register_module(nameBuffer, DecoderBlock(blockIn, blockOut, numResBlocksPerEntry, doUpSample, normGroups, dropout, nonLinearity)));

		blockIn = blockOut;
	}

	//	NL + channels down
	m_seq->push_back(register_module("groupNormFinal", torch::nn::GroupNorm(normGroups, blockIn)));
	AddNonLinearityModule(m_seq, nonLinearity, "NLFinal");
	m_seq->push_back(register_module("convFinal1", torch::nn::Conv2d(torch::nn::Conv2dOptions(blockIn, outputChannels, 3).padding(1))));
}

torch::Tensor Decoder2DImpl::forward(torch::Tensor x)
{
	return m_seq->forward(x);
}
