#include "NNNoiseFinder.h"

#include "AnimGenBoneDictionary.h"
#include "AnimGenDescDictionary.h"

#include "NNUNET.h"
#include "NNTextEncoder.h"
#include "NNCLIP.h"

//
NNNoiseFinderImpl*	g_NNNoiseFinderImpl = nullptr;

NNNoiseFinderImpl::NNNoiseFinderImpl():
	m_seq(register_module("m_seq", torch::nn::Sequential())),
	m_timeInputSeq(register_module("m_timeInputSeq", torch::nn::Sequential())),
	m_loss(register_module("m_loss", NNNoiseFinderLoss()))
{
	Assert(g_NNNoiseFinderImpl == nullptr);
	g_NNNoiseFinderImpl = this;

	//
	int					latentChannels = 16;
	int					channelGroups = 32;
	//int					hiddenSize = 320;
	int					hiddenSize = 128;
	int					normSize = channelGroups;
	double				dropout = 0.2;

	int					timeSize = hiddenSize;
	int					timeEmbedSize = timeSize*4;

	int					numHeads = 8;
	//int					textEmbedSize = NNTextEncoderImpl::kEmbedSize;
	int					textEmbedSize = NNCLIPImpl::kTextEmbedSize;	

	int					numResBlocksPerEntry = 2;
	std::vector<int>	channelMult = {1, 2, 4, 4};
	std::vector<int>	attentionResolutions = {4, 2, 1};

	//
	m_timeInputSeq->push_back(register_module("timeInputPosition", NNUNETTimePositioning(timeSize)));
	m_timeInputSeq->push_back(register_module("timeInputEmbed", NNUNETTimeEmbedding(timeSize, timeEmbedSize)));

	//	this is pretty close to the UNET setup
	m_seq->push_back(register_module("multi", NNUNET(latentChannels, hiddenSize, numHeads, textEmbedSize, timeEmbedSize, normSize, dropout, channelMult, numResBlocksPerEntry, attentionResolutions, kNL_Silu)));
}

NNNoiseFinderImpl::~NNNoiseFinderImpl()
{
	Assert(g_NNNoiseFinderImpl == this);
	g_NNNoiseFinderImpl = nullptr;
}

torch::Tensor NNNoiseFinderImpl::forward(torch::Tensor x, torch::Tensor context, torch::Tensor time)
{
	time = m_timeInputSeq->forward(time);

	torch::Tensor	estimatedNoise = m_seq->forward(x, context, time);

	return estimatedNoise;
}

void NNNoiseFinderImpl::CalcLoss(torch::Tensor predictedNoise, torch::Tensor baseNoise, torch::Tensor timeSteps, torch::Tensor* lossOut, std::string* logOut)
{
	m_loss->forward(predictedNoise, baseNoise, timeSteps, lossOut, logOut);
}