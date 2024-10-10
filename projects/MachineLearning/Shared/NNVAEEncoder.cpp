#include "NNVAEEncoder.h"

#include "AnimGenHyperParameters.h"

#include "AttentionMulti.h"
#include "AnimGenBoneDictionary.h"

#include "NNResConv2DMulti.h"

#include "NNUtilityBlocks.h"

NNVAEEncoderImpl::NNVAEEncoderImpl(int transformVectorSize):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	int	keyframeCount = AnimGenHyperParameters::kMaxKeyframeSequenceLength;	

	//	use learning rate 7.8125e-06
	/*int		inputSize = keyframeCount;
	int		outputSize = keyframeCount*2;
	int		hiddenSize = keyframeCount*4;
	int		numHeads = 32;
	int		numBlocks = 6;
	int		normSize = 4;
	double	dropout = 0.2;
	m_seq->push_back(register_module("resMulti", ResConv1DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Tanh)));*/

	int		baseSize = g_animGenBoneDictionary->GetVocabSize() * transformVectorSize;

	/*int		inputSize = baseSize;
	int		outputSize = baseSize*2;
	int		hiddenSize = baseSize*4;
	int		numBlocks = 3;
	int		normSize = transformVectorSize;
	double	dropout = 0.5;*/
	//m_seq->push_back(register_module("resMulti", ResConv1DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));

	//m_seq->push_back(register_module("layerNormInput", torch::nn::LayerNorm(torch::nn::LayerNormOptions({1}))));	//	nonsense to norm this on a size of 1?
	//AddNonLinearityModule(m_seq, kNL_Silu, "NLInput");
	//m_seq->push_back(register_module("linearInput", torch::nn::Linear(1, 8)));
	//m_seq->push_back(register_module("resMulti", ResConv2DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));

	//m_seq->push_back(register_module("resMulti", ResConv2DSplitUpsizeMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));

	/*int		inputSize = transformVectorSize;
	int		outputSize = 8*2;
	int		hiddenSize = 128;
	int		numBlocks = 8;
	int		normSize = 32;
	double	dropout = 0.0;*/
	//m_seq->push_back(register_module("transposeBK", Transpose13Layer()));
	//m_seq->push_back(register_module("resMulti", ResConv2DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));

	//m_seq->push_back(register_module("resMulti", SDResConv2DMultiEncoder(inputSize, outputSize, hiddenSize, normSize, dropout, kNL_Silu)));

	//m_seq->push_back(register_module("transposeBK", Transpose23Layer()));
	//m_seq->push_back(register_module("linearInput", torch::nn::Linear(g_animGenBoneDictionary->GetVocabSize(), 128)));
	//m_seq->push_back(register_module("resMulti", ResConv2DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));

	//m_seq->push_back(register_module("transposeBK", Transpose13Layer()));
	//m_seq->push_back(register_module("linearInput", torch::nn::Linear(g_animGenBoneDictionary->GetVocabSize(), 128)));
	//m_seq->push_back(register_module("resMulti", SDResConv2DMultiEncoder(inputSize, outputSize, hiddenSize, normSize, dropout, kNL_Silu)));

	//m_seq->push_back(register_module("transposeBK", Transpose13Layer()));
	//m_seq->push_back(register_module("linearInput", torch::nn::Linear(g_animGenBoneDictionary->GetVocabSize(), 128)));
	//m_seq->push_back(register_module("resMulti", SDResConv2DMultiEncoder(inputSize, outputSize, hiddenSize, normSize, dropout, kNL_Silu)));

	std::vector<int>	channelMult = {1, 2, 4};
	//std::vector<int>	channelMult = {1, 2, 4, 4};
	//std::vector<int>	channelMult = {1, 2, 4, 8};
	int					numResBlocksPerEntry = 2;

	int					latentChannels = 16;
	int					channelGroups = 32;
	//int					channelGroups = 16;
	int					inputSize = transformVectorSize;
	int					outputSize = latentChannels*2;
	//int					hiddenSize = latentChannels * channelGroups;
	int					hiddenSize = 128;
	int					normSize = channelGroups;
	int					remapDataInput = g_animGenBoneDictionary->GetVocabSize();
	int					remapDataSize = 128;
	double				dropout = 0.2;
	m_seq->push_back(register_module("transpose", Transpose13Layer()));
	m_seq->push_back(register_module("resMulti", NNResConv2DMultiEncoder(inputSize, outputSize, hiddenSize, remapDataInput, remapDataSize, normSize, dropout, channelMult, numResBlocksPerEntry, kNL_Silu)));


	//int	numBlocks = 8;
	//m_seq->push_back(register_module("transpose", Transpose13Layer()));
	//m_seq->push_back(register_module("linearInput", torch::nn::Linear(remapDataInput, remapDataSize)));
	//m_seq->push_back(register_module("resMulti", ResConv2DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));
}

torch::Tensor NNVAEEncoderImpl::forward(torch::Tensor animImage, torch::Tensor* logVarOut, torch::Tensor* meanOut)
{
	//	inputs 
	//	animImage		: should be {batch, bones, keyframes, transformVectorSize}

	//	outputs
	//	network			: {batch, keyframes, kLatentAnimDescSize*2}
	//	final			: {batch, keyframes, kLatentAnimDescSize}

	s64	batchSize = animImage.size(0);
	torch::Tensor	x = animImage;

	//	normalize euler angles
	//x = x * (1.0f / M_PI);

	//	{batch, 6, bones, keyframes} -> {batch, 6*bones, keyframes}
	//x = animImage.view({animImage.size(0), animImage.size(1)*animImage.size(2), animImage.size(3)});

	//	{batch, 6*bones, keyframes} -> {batch, keyframes, 6*bones}
	//x = x.transpose(1, 2).contiguous();
	//x = x.unsqueeze(-1);

	//	{batch, keyframes, 6*bones} -> {batch, keyframes, 6*bones*2}
	x = m_seq->forward(x);

	enum
	{
		kChunk_Mean,
		kChunk_LogVariance,

		kNumChunks
	};
	std::vector<torch::Tensor>	chunkComponents = torch::chunk(x, kNumChunks, 1);

	chunkComponents[kChunk_LogVariance] = torch::clamp(chunkComponents[kChunk_LogVariance], -30, 20);	//	why -30 and 20?
	torch::Tensor	variance = chunkComponents[kChunk_LogVariance].exp();
	torch::Tensor	stdDev = variance.sqrt();

	//	the encoder creates a distribution for the data in the input, and then samples from the distribution randomly using this noise to create the initial latent version of it?
	//	is that how this works?
	torch::Tensor	noise = torch::randn(stdDev.sizes(), x.device());

	x = stdDev * noise + chunkComponents[kChunk_Mean];
	//x = chunkComponents[kChunk_Mean];

	//	scale the output by a constant, dunno why this one
	//	probably make it easier to fit an appropriate distribution in SD
	//	actually, I think this must have only been used in inference, becuase it's nonsensical during training if it doesn't apply to the output mean and stdDev
	//x = x * 0.18215;

	if( logVarOut )
	{
		*logVarOut = chunkComponents[kChunk_LogVariance];
	}

	if( meanOut )
	{
		*meanOut = chunkComponents[kChunk_Mean];
	}

	return x;
}