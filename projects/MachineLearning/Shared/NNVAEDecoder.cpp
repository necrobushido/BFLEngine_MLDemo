#include "NNVAEDecoder.h"

#include "AnimGenHyperParameters.h"

#include "AttentionMulti.h"
#include "AnimGenBoneDictionary.h"

#include "NNResConv2DMulti.h"

#include "NNUtilityBlocks.h"

NNVAEDecoderImpl::NNVAEDecoderImpl(int transformVectorSize):
	m_seq(register_module("m_seq", torch::nn::Sequential())),
	m_transformVectorSize(transformVectorSize)
{
	int	keyframeCount = AnimGenHyperParameters::kMaxKeyframeSequenceLength;

	//
	/*int		inputSize = keyframeCount;
	int		outputSize = keyframeCount;
	int		hiddenSize = keyframeCount*4;
	int		numHeads = 32;
	int		numBlocks = 6;
	int		normSize = 4;
	double	dropout = 0.2;
	m_seq->push_back(register_module("resMulti", ResConv1DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Tanh)));*/

	int		baseSize = g_animGenBoneDictionary->GetVocabSize() * transformVectorSize;

	//int		inputSize = baseSize;
	//int		outputSize = baseSize;
	//int		hiddenSize = baseSize*4;
	//int		numBlocks = 3;
	//int		normSize = transformVectorSize;
	//double	dropout = 0.5;
	//m_seq->push_back(register_module("resMulti", ResConv1DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));
	
	//m_seq->push_back(register_module("resMulti", ResConv2DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));
	//m_seq->push_back(register_module("layerNormOutput", torch::nn::LayerNorm(torch::nn::LayerNormOptions({8}))));
	//AddNonLinearityModule(m_seq, kNL_Silu, "NLOutput");
	//m_seq->push_back(register_module("linearOutput", torch::nn::Linear(8, 1)));

	//m_seq->push_back(register_module("resMulti", ResConv2DSplitDownsizeMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));

	/*int		inputSize = 8;
	int		outputSize = transformVectorSize;
	int		hiddenSize = 128;
	int		numBlocks = 8;
	int		normSize = 32;
	double	dropout = 0.0;*/
	//m_seq->push_back(register_module("resMulti", ResConv2DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));
	//m_seq->push_back(register_module("transposeBK", Transpose13Layer()));

	//m_seq->push_back(register_module("resMulti", SDResConv2DMultiDecoder(inputSize, outputSize, hiddenSize, normSize, dropout, kNL_Silu)));

	/*m_seq->push_back(register_module("resMulti", ResConv2DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));
	m_seq->push_back(register_module("layerNormOutput", torch::nn::LayerNorm(torch::nn::LayerNormOptions({128}))));
	AddNonLinearityModule(m_seq, kNL_Silu, "NLOutput");
	m_seq->push_back(register_module("linearOutput", torch::nn::Linear(128, g_animGenBoneDictionary->GetVocabSize())));
	m_seq->push_back(register_module("transposeBK", Transpose23Layer()));*/

	//m_seq->push_back(register_module("resMulti", SDResConv2DMultiDecoder(inputSize, outputSize, hiddenSize, normSize, dropout, kNL_Silu)));
	//m_seq->push_back(register_module("layerNormOutput", torch::nn::LayerNorm(torch::nn::LayerNormOptions({128}))));
	//AddNonLinearityModule(m_seq, kNL_Silu, "NLOutput");
	//m_seq->push_back(register_module("linearOutput", torch::nn::Linear(128, g_animGenBoneDictionary->GetVocabSize())));
	//m_seq->push_back(register_module("transposeBK", Transpose13Layer()));

	std::vector<int>	channelMult = {1, 2, 4};
	//std::vector<int>	channelMult = {1, 2, 4, 4};
	//std::vector<int>	channelMult = {1, 2, 4, 8};
	int					numResBlocksPerEntry = 2;

	int					latentChannels = 16;
	int					channelGroups = 32;
	//int					channelGroups = 16;
	int					inputSize = latentChannels;
	int					outputSize = transformVectorSize;
	//int					hiddenSize = latentChannels * channelGroups;
	int					hiddenSize = 128;
	int					normSize = channelGroups;
	int					remapDataInput = g_animGenBoneDictionary->GetVocabSize();
	int					remapDataSize = 128;
	double				dropout = 0.2;
	m_seq->push_back(register_module("resMulti", NNResConv2DMultiDecoder(inputSize, outputSize, hiddenSize, remapDataInput, remapDataSize, normSize, dropout, channelMult, numResBlocksPerEntry, kNL_Silu)));
	m_seq->push_back(register_module("transpose", Transpose13Layer()));

	//int	numBlocks = 8;
	//m_seq->push_back(register_module("resMulti", ResConv2DMulti(inputSize, outputSize, hiddenSize, numBlocks, normSize, dropout, kNL_Silu)));
	//m_seq->push_back(register_module("linearOutput", torch::nn::Linear(remapDataSize, remapDataInput)));
	//m_seq->push_back(register_module("transpose", Transpose13Layer()));
}

torch::Tensor NNVAEDecoderImpl::forward(torch::Tensor latentAnimDesc)
{
	//	inputs 
	//	latentAnimDesc	: should be 
	
	//	outputs
	//	{batch, bones, keyframes, transformVectorSize}

	torch::Tensor	x = latentAnimDesc;

	//	reverse the scale from the encoder
	//x = x * (1.0 / 0.18215);

	//	{batch, keyframes, transformVectorSize*bones} -> {batch, keyframes, transformVectorSize*bones}
	x = m_seq->forward(x);

	//	{{batch, keyframes, transformVectorSize*bones} -> {batch, transformVectorSize*bones, keyframes}
	//x = x.transpose(1, 2).contiguous();
	//x = x.squeeze(-1);

	//	{batch, transformVectorSize*bones, keyframes} -> {batch, transformVectorSize, bones, keyframes}
	//x = x.view({x.size(0), m_transformVectorSize, x.size(1)/m_transformVectorSize, x.size(2)});

	//	denormalize euler angles
	//x = x * (M_PI);
	
	return x;
}