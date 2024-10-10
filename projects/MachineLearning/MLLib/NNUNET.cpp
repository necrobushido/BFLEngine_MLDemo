#include "NNUNET.h"

#include "SelfAttention.h"
#include "CrossAttention.h"

#include "NNUtilityBlocks.h"
#include "MathNamespace.h"

//
NNUNETTimePositioningImpl::NNUNETTimePositioningImpl(int size):
	m_size(size)
{
	int				elementCount = m_size / 2;	
	//torch::Tensor	freqs = torch::pow(10000, -torch::arange(0, elementCount, torch::kF32) / (f32)elementCount);
	//torch::Tensor	freqs = torch::exp(torch::arange(0, m_size, 2) * (-Math::LN(10000.0) / m_size));
	torch::Tensor	freqs = torch::exp(torch::arange(0, elementCount, torch::kF32) * (-Math::LN(10000.0) / elementCount));
	freqs = freqs.unsqueeze(0);

	m_freqs = register_buffer("m_freqs", freqs);
}

torch::Tensor NNUNETTimePositioningImpl::forward(torch::Tensor trainingTimestepIndices)
{
	s64				batchSize = trainingTimestepIndices.size(0);
	torch::Tensor	expandedFreqs = m_freqs.expand({batchSize, m_freqs.size(1)});
	torch::Tensor	x = trainingTimestepIndices.to(torch::kFloat).unsqueeze(1) * expandedFreqs;

	torch::Tensor	result = torch::cat({torch::cos(x), torch::sin(x)}, -1);

	/*torch::Tensor	evensPos = torch::sin(x);
	torch::Tensor	oddsPos = torch::cos(x);
	torch::Tensor	result = torch::zeros({batchSize, m_size}, trainingTimestepIndices.device());
	result.slice(1, 0, m_size, 2) = evensPos;
	result.slice(1, 1, m_size, 2) = oddsPos;*/

	return result;
}

//
NNUNETTimeEmbeddingImpl::NNUNETTimeEmbeddingImpl(int timeSize, int timeEmbedSize, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	m_seq->push_back(register_module("linear1", torch::nn::Linear(timeSize, timeEmbedSize)));
	AddNonLinearityModule(m_seq, nonLinearity, "NL");
	m_seq->push_back(register_module("linear2", torch::nn::Linear(timeEmbedSize, timeEmbedSize)));
}

torch::Tensor NNUNETTimeEmbeddingImpl::forward(torch::Tensor x)
{
	return m_seq->forward(x);
}

//
class NNUNETMultiResBlockImpl : public torch::nn::Module 
{
public:
    NNUNETMultiResBlockImpl(int inputChannels, int outputChannels, int timeSize, int normSize, double dropout, eNonLinearity nonLinearity = kNL_Silu):
		m_seq1(register_module("m_seq1", torch::nn::Sequential())),
		m_seq2(register_module("m_seq2", torch::nn::Sequential())),
		m_timeSeq(register_module("m_timeSeq", torch::nn::Sequential())),
		m_residualSeq(register_module("m_residualSeq", torch::nn::Sequential()))
	{
		m_seq1->push_back(register_module("groupNorm_1", torch::nn::GroupNorm(normSize, inputChannels)));
		AddNonLinearityModule(m_seq1, nonLinearity, "NL");
		m_seq1->push_back(register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, outputChannels, 3).padding(1))));

		m_seq2->push_back(register_module("groupNorm_2", torch::nn::GroupNorm(normSize, outputChannels)));
		AddNonLinearityModule(m_seq2, nonLinearity, "NL");
		m_seq2->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));	//	SD had this here


		torch::nn::Conv2d	conv2 = torch::nn::Conv2d(torch::nn::Conv2dOptions(outputChannels, outputChannels, 3).padding(1));
		torch::nn::init::zeros_(conv2->weight);
		torch::nn::init::zeros_(conv2->bias);
		m_seq2->push_back(register_module("conv2", conv2));
		//m_seq2->push_back(register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(outputChannels, outputChannels, 3).padding(1))));	//	this was fully initialized to zero in SD and I do not understand why
		//m_seq2->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));

		if( inputChannels == outputChannels )
		{
			m_residualSeq->push_back(register_module("residualLayer", torch::nn::Identity()));
		}
		else
		{
			m_residualSeq->push_back(register_module("residualLayer", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, outputChannels, 1).padding(0))));
		}

		//	does this need a normalization layer?  the original didn't have one
		AddNonLinearityModule(m_timeSeq, nonLinearity, "NL_time");
		m_timeSeq->push_back(register_module("linear_time", torch::nn::Linear(timeSize, outputChannels)));
	}

    torch::Tensor forward(torch::Tensor x, torch::Tensor context, torch::Tensor time)
	{
		//	context unused

		torch::Tensor	residual = m_residualSeq->forward(x);

		x = m_seq1->forward(x) + m_timeSeq->forward(time).unsqueeze(-1).unsqueeze(-1);
		x = m_seq2->forward(x) + residual;

		return x;
	}

private:
	torch::nn::Sequential	m_seq1;
	torch::nn::Sequential	m_seq2;
	torch::nn::Sequential	m_timeSeq;
	torch::nn::Sequential	m_residualSeq;
};

TORCH_MODULE(NNUNETMultiResBlock);

//
class NNUNETAttentionBlockImpl : public torch::nn::Module
{
public:
	NNUNETAttentionBlockImpl(int numHeads, int embedSize, int contextSize, int normSize, double dropout):
		m_inputSeq(register_module("m_inputSeq", torch::nn::Sequential())),
		m_selfAttentionSeq(register_module("m_selfAttentionSeq", torch::nn::Sequential())),
		m_crossAttentionLayerNorm(register_module("m_crossAttentionLayerNorm", torch::nn::LayerNorm(torch::nn::LayerNormOptions({embedSize})))),
		m_crossAttention(register_module("m_crossAttention", CrossAttention(numHeads, embedSize, contextSize, false, true, false))),
		m_gegluLayerNorm(register_module("m_gegluLayerNorm", torch::nn::LayerNorm(torch::nn::LayerNormOptions({embedSize})))),
		m_gegluLinear1(register_module("m_gegluLinear1", torch::nn::Linear(embedSize, embedSize * 4 * 2))),
		m_gegluLinear2(register_module("m_gegluLinear2", torch::nn::Linear(embedSize * 4, embedSize))),
		m_outputSeq(register_module("m_outputSeq", torch::nn::Sequential())),
		m_crossDropout(register_module("m_crossDropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout)))),	//	dropout doesn't have parameters right?  could just use one of these layers for the whole thing then right?
		m_gegluDropout(register_module("m_gegluDropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))))
	{
		m_inputSeq->push_back(register_module("normInput", torch::nn::GroupNorm(normSize, embedSize)));
		m_inputSeq->push_back(register_module("convInput", torch::nn::Conv2d(torch::nn::Conv2dOptions(embedSize, embedSize, 1).padding(0))));
		//m_inputSeq->push_back(register_module("transposeInput", Transpose12Layer()));

		m_selfAttentionSeq->push_back(register_module("attentionLayerNorm", torch::nn::LayerNorm(torch::nn::LayerNormOptions({embedSize}))));
		m_selfAttentionSeq->push_back(register_module("selfAttention", SelfAttention(numHeads, embedSize, false, true, false, dropout)));
		//m_selfAttentionSeq->push_back(register_module("selfAttention", MHSelfAttention(numHeads, embedSize, 0.0, false)));	//	this doesn't seem to work
		m_selfAttentionSeq->push_back(register_module("attentionDropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));

		//m_outputSeq->push_back(register_module("transposeOutput", Transpose12Layer()));
		//m_outputSeq->push_back(register_module("normOutput", torch::nn::GroupNorm(normSize, embedSize)));

		torch::nn::Conv2d	convOutput = torch::nn::Conv2d(torch::nn::Conv2d(torch::nn::Conv2dOptions(embedSize, embedSize, 1).padding(0)));
		torch::nn::init::zeros_(convOutput->weight);
		torch::nn::init::zeros_(convOutput->bias);
		m_outputSeq->push_back(register_module("convOutput", convOutput));
		//m_outputSeq->push_back(register_module("convOutput", torch::nn::Conv2d(torch::nn::Conv2dOptions(embedSize, embedSize, 1).padding(0))));	//	this was fully initialized to zero in SD and I do not understand why

		m_outputSeq->push_back(register_module("dropoutOutput", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));
	}

    torch::Tensor forward(torch::Tensor x, torch::Tensor context, torch::Tensor time)
	{
		//	inputs 
		//	x				: should be {batch, keyframes, bones*6}
		//	context			: should be {batch, tokens, embed}
		//	time unused

		//	outputs
		//	should be {batch, keyframes, bones*6}

		torch::Tensor	longResidue = x;

		//	{batch, keyframes, bones*6} -> {batch, bones*6, keyframes}
		x = m_inputSeq->forward(x);

		x = x.view({longResidue.size(0), longResidue.size(1), longResidue.size(2)*longResidue.size(3)});
		x = x.transpose(1, 2).contiguous();

		torch::Tensor	shortResidue = x;
		//	this eats a lot of VRAM
		x = m_selfAttentionSeq->forward(x);
		x = x + shortResidue;

		shortResidue = x;
		x = m_crossAttentionLayerNorm->forward(x);
		x = m_crossAttention->forward(x, context);
		x = m_crossDropout->forward(x);
		x = x + shortResidue;

		shortResidue = x;
		x = m_gegluLayerNorm->forward(x);
		enum
		{
			kValue,
			kGate,
			kNumSplits
		};
		std::vector<torch::Tensor>	gegluComponents = m_gegluLinear1->forward(x).chunk(kNumSplits, -1);
		x = gegluComponents[kValue] * torch::nn::functional::gelu(gegluComponents[kGate]);
		x = m_gegluDropout->forward(x);	//	SD had this here
		x = m_gegluLinear2->forward(x);
		//x = m_gegluDropout->forward(x);
		x = x + shortResidue;

		x = x.transpose(1, 2).contiguous();
		x = x.view({longResidue.size(0), longResidue.size(1), longResidue.size(2), longResidue.size(3)});

		x = m_outputSeq->forward(x);

		return x + longResidue;
	}

private:
	torch::nn::Sequential	m_inputSeq;
	torch::nn::Sequential	m_selfAttentionSeq;
	torch::nn::LayerNorm	m_crossAttentionLayerNorm;
	CrossAttention			m_crossAttention;
	torch::nn::LayerNorm	m_gegluLayerNorm;
	torch::nn::Linear		m_gegluLinear1;
	torch::nn::Linear		m_gegluLinear2;
	torch::nn::Sequential	m_outputSeq;
	torch::nn::Dropout		m_gegluDropout;
	torch::nn::Dropout		m_crossDropout;
};

TORCH_MODULE(NNUNETAttentionBlock);

//	this is stupid
class NNUNETSequentialImpl : public torch::nn::SequentialImpl
{
public:
	torch::Tensor forward(torch::Tensor x, torch::Tensor context, torch::Tensor time) 
	{
		for( torch::nn::SequentialImpl::Iterator it = begin(); it != end(); ++it)
		{
			if( NNUNETAttentionBlockImpl* attentionBlock = dynamic_cast<NNUNETAttentionBlockImpl*>(it->ptr().get()) )
			{
				x = attentionBlock->forward(x, context, time);
			} 
			else if( NNUNETMultiResBlockImpl* residualBlock = dynamic_cast<NNUNETMultiResBlockImpl*>(it->ptr().get()) )
			{
				x = residualBlock->forward(x, context, time);
			}
			else 
			{
				x = it->forward(x);
			}
		}

		return x;
	}
};

#define SEQ_ADD_MODULE(x, y, z) ((x)->push_back((x)->register_module((y), (z))))
//#define SEQ_ADD_MODULE(x, y, z) ((x)->push_back(register_module((y), (z))))

bool VectorContains(const std::vector<int> vec, int valueToTest)
{
	bool	result = false;
	for(int i = 0; i < (int)vec.size() && !result; ++i)
	{
		result = vec[i] == valueToTest;
	}

	return result;
}

//
NNUNETImpl::NNUNETImpl(int inputChannels, int hiddenChannels, int numHeads, int textCrossSize, int timeSize, int normSize, double dropout, std::vector<int> channelMult, int numResBlocksPerEntry, std::vector<int> attentionResolutions, eNonLinearity nonLinearity):
	m_outputSeq(register_module("m_outputSeq", torch::nn::Sequential())),
	m_encoderSeq(register_module("m_encoderSeq", torch::nn::Sequential())),
	m_centerSeq(register_module("m_centerSeq", torch::nn::Sequential())),
	m_decoderSeq(register_module("m_decoderSeq", torch::nn::Sequential()))
{
	int		numLevels = (int)channelMult.size();

	int		currentChannels = hiddenChannels;	//	ch
	int		ds = 1;

	int		nameId = 0;
	char	nameBuffer[256];

	std::stack<int>	inputBlockChannels;
		
	{
		std::shared_ptr<NNUNETSequentialImpl>	esInput = std::make_shared<NNUNETSequentialImpl>();
		SEQ_ADD_MODULE(esInput, "convInput", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, currentChannels, 3).padding(1)));
		m_encoderSeq->push_back(register_module("esInput", esInput));

		inputBlockChannels.push(currentChannels);

		for(int level = 0; level < numLevels; ++level)
		{
			int	mult = channelMult[level];

			for(int blockIdx = 0; blockIdx < numResBlocksPerEntry; ++blockIdx)
			{
				std::shared_ptr<NNUNETSequentialImpl>	thisES = std::make_shared<NNUNETSequentialImpl>();

				sprintf(nameBuffer, "name_%d", nameId++);
				SEQ_ADD_MODULE(thisES, nameBuffer, NNUNETMultiResBlock(currentChannels, mult * hiddenChannels, timeSize, normSize, dropout, nonLinearity));

				currentChannels = mult * hiddenChannels;

				if( VectorContains(attentionResolutions, ds) )
				{
					sprintf(nameBuffer, "name_%d", nameId++);
					SEQ_ADD_MODULE(thisES, nameBuffer, NNUNETAttentionBlock(numHeads, currentChannels, textCrossSize, normSize, dropout));
				}
					
				sprintf(nameBuffer, "ES_%d_%d", level, blockIdx);
				m_encoderSeq->push_back(register_module(nameBuffer, thisES));
				inputBlockChannels.push(currentChannels);
			}

			if( level < numLevels - 1 )
			{
				int		outChannels = currentChannels;					

				//	downsample
				std::shared_ptr<NNUNETSequentialImpl>	thisES = std::make_shared<NNUNETSequentialImpl>();

				sprintf(nameBuffer, "name_%d", nameId++);
				SEQ_ADD_MODULE(thisES, nameBuffer, torch::nn::Conv2d(torch::nn::Conv2dOptions(currentChannels, outChannels, 3).stride(2).padding(1)));

				sprintf(nameBuffer, "ES_downsample_%d", level);
				m_encoderSeq->push_back(register_module(nameBuffer, thisES));

				currentChannels = outChannels;
				inputBlockChannels.push(currentChannels);

				ds *= 2;
			}
		}
	}

	{
		std::shared_ptr<NNUNETSequentialImpl>	centerBlock = std::make_shared<NNUNETSequentialImpl>();
		sprintf(nameBuffer, "name_%d", nameId++);
		SEQ_ADD_MODULE(centerBlock, nameBuffer, NNUNETMultiResBlock(currentChannels, currentChannels, timeSize, normSize, dropout, nonLinearity));

		sprintf(nameBuffer, "name_%d", nameId++);
		SEQ_ADD_MODULE(centerBlock, nameBuffer, NNUNETAttentionBlock(numHeads, currentChannels, textCrossSize, normSize, dropout));

		sprintf(nameBuffer, "name_%d", nameId++);
		SEQ_ADD_MODULE(centerBlock, nameBuffer, NNUNETMultiResBlock(currentChannels, currentChannels, timeSize, normSize, dropout, nonLinearity));

		sprintf(nameBuffer, "centerBlockS");
		m_centerSeq->push_back(register_module(nameBuffer, centerBlock));
	}

	{
		for(int level = numLevels - 1; level >= 0; --level)
		{
			int	mult = channelMult[level];

			for(int blockIdx = 0; blockIdx < numResBlocksPerEntry + 1; ++blockIdx)	//	one more res block than encoder
			{
				int	skipChannels = inputBlockChannels.top();
				inputBlockChannels.pop();

				char	nameBuffer[256];					

				std::shared_ptr<NNUNETSequentialImpl>	thisDS = std::make_shared<NNUNETSequentialImpl>();

				sprintf(nameBuffer, "name_%d", nameId++);
				SEQ_ADD_MODULE(thisDS, nameBuffer, NNUNETMultiResBlock(currentChannels + skipChannels, mult * hiddenChannels, timeSize, normSize, dropout, nonLinearity));

				currentChannels = mult * hiddenChannels;

				if( VectorContains(attentionResolutions, ds) )
				{
					sprintf(nameBuffer, "name_%d", nameId++);
					SEQ_ADD_MODULE(thisDS, nameBuffer, NNUNETAttentionBlock(numHeads, currentChannels, textCrossSize, normSize, dropout));
				}

				if( level != 0 && 
					blockIdx == numResBlocksPerEntry )
				{
					int		outChannels = currentChannels;

					sprintf(nameBuffer, "name_%d", nameId++);
					SEQ_ADD_MODULE(thisDS, nameBuffer, InterpolateBlock(2, 2));

					sprintf(nameBuffer, "name_%d", nameId++);
					SEQ_ADD_MODULE(thisDS, nameBuffer, torch::nn::Conv2d(torch::nn::Conv2dOptions(currentChannels, outChannels, 3).padding(1)));

					ds /= 2;
				}

				sprintf(nameBuffer, "DS_%d_%d", level, blockIdx);
				m_decoderSeq->push_back(register_module(nameBuffer, thisDS));
			}
		}
	}

	m_outputSeq->push_back(register_module("normOutput", torch::nn::GroupNorm(normSize, currentChannels)));
	AddNonLinearityModule(m_outputSeq, nonLinearity, "NLOutput");

	torch::nn::Conv2d	outputConv = torch::nn::Conv2d(torch::nn::Conv2dOptions(currentChannels, inputChannels, 3).padding(1));
	torch::nn::init::zeros_(outputConv->weight);
	torch::nn::init::zeros_(outputConv->bias);
	m_outputSeq->push_back(register_module("convOutput", outputConv));
	//m_outputSeq->push_back(register_module("convOutput", torch::nn::Conv2d(torch::nn::Conv2dOptions(currentChannels, inputChannels, 3).padding(1))));	//	this was fully initialized to zero in SD and I do not understand why

	//	the zero may be related to "Adding Conditional Control To Text To Image DiffusionModels" which talks about using zero convolutions to tie layers together and remove noise, but it doesn't seem to map 1-to-1 with this so I'm still not sure
}

torch::Tensor NNUNETImpl::forward(torch::Tensor x, torch::Tensor context, torch::Tensor time)
{
	std::stack<torch::Tensor>	skipConnections;
	for( torch::nn::SequentialImpl::Iterator it = m_encoderSeq->begin(); it != m_encoderSeq->end(); ++it)
	{
		x = it->forward(x, context, time);
		
		skipConnections.push(x);
	}

	for( torch::nn::SequentialImpl::Iterator it = m_centerSeq->begin(); it != m_centerSeq->end(); ++it)
	{
		x = it->forward(x, context, time);
	}
		
	for( torch::nn::SequentialImpl::Iterator it = m_decoderSeq->begin(); it != m_decoderSeq->end(); ++it)
	{
		x = torch::cat({x, skipConnections.top()}, 1);
		skipConnections.pop();
		
		x = it->forward(x, context, time);
	}

	//x = torch::cat({x, skipAll}, 1);
	x = m_outputSeq->forward(x);
		
	return x;
}