#include "AttentionMulti.h"
#include "types.h"

#include "SelfAttention.h"

//
AttentionMultiBlockImpl::AttentionMultiBlockImpl(int numHeads, int embedSize, double dropout, bool causal, eNonLinearity nonLinearity):
	m_attentionSeq(register_module("m_attentionSeq", torch::nn::Sequential())),
	m_nlSeq(register_module("m_nlSeq", torch::nn::Sequential()))
{
	m_attentionSeq->push_back(register_module("layerNorm1", torch::nn::LayerNorm(torch::nn::LayerNormOptions({embedSize}))));
	m_attentionSeq->push_back(register_module("selfAttention", SelfAttention(numHeads, embedSize, false, false, causal, dropout)));
	//m_attentionSeq->push_back(register_module("selfAttention", FlashSelfAttention(numHeads, embedSize, false, false, causal, dropout)));
	m_attentionSeq->push_back(register_module("dropout1", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));

	m_nlSeq->push_back(register_module("layerNorm2", torch::nn::LayerNorm(torch::nn::LayerNormOptions({embedSize}))));
	m_nlSeq->push_back(register_module("linear1", torch::nn::Linear(embedSize, embedSize * 4)));
	AddNonLinearityModule(m_nlSeq, nonLinearity, "NL");
	m_nlSeq->push_back(register_module("linear2", torch::nn::Linear(embedSize * 4, embedSize)));
	m_nlSeq->push_back(register_module("dropout2", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));
}

torch::Tensor AttentionMultiBlockImpl::forward(torch::Tensor x)
{
	//	single residue around the whole thing
	/*torch::Tensor	longResidue = x;
	x = m_attentionSeq->forward(x);
	x = m_nlSeq->forward(x);
	return x + longResidue;*/

	//	residue around each chunk
	x = x + m_attentionSeq->forward(x);
	x = x + m_nlSeq->forward(x);
	return x;
}

//
AttentionMultiImpl::AttentionMultiImpl(int numHeads, int hiddenLayerSize, int hiddenLayerCount, bool causal, double dropout, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	//Assert(hiddenLayerCount > 0);
	
	for(int i = 0; i < hiddenLayerCount; ++i)
	{
		char	nameBuffer[256];
		sprintf(nameBuffer, "AttentionMultiBlock%d", i);
		m_seq->push_back(register_module(nameBuffer, AttentionMultiBlock(numHeads, hiddenLayerSize, dropout, causal, nonLinearity)));
	}

	m_seq->push_back(register_module("layerNormOutput", torch::nn::LayerNorm(torch::nn::LayerNormOptions({hiddenLayerSize}))));
}

AttentionMultiImpl::AttentionMultiImpl(int inputSize, int outputSize, int numHeads, int hiddenLayerSize, int hiddenLayerCount, bool causal, double dropout, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	//Assert(hiddenLayerCount > 0);
	
	//Assert(inputSize == hiddenLayerSize);
	m_seq->push_back(register_module("layerNormInput", torch::nn::LayerNorm(torch::nn::LayerNormOptions({inputSize}))));
	m_seq->push_back("linearInput", torch::nn::Linear(inputSize, hiddenLayerSize));

	for(int i = 0; i < hiddenLayerCount; ++i)
	{
		char	nameBuffer[256];
		sprintf(nameBuffer, "AttentionMultiBlock%d", i);
		m_seq->push_back(register_module(nameBuffer, AttentionMultiBlock(numHeads, hiddenLayerSize, dropout, causal, nonLinearity)));
	}

	m_seq->push_back(register_module("layerNormOutput", torch::nn::LayerNorm(torch::nn::LayerNormOptions({hiddenLayerSize}))));
	m_seq->push_back("linearOutput", torch::nn::Linear(hiddenLayerSize, outputSize));	
	//m_seq->push_back(register_module("linear1Output", torch::nn::Linear(hiddenLayerSize, hiddenLayerSize * 4)));
	//AddNonLinearityModule(m_seq, nonLinearity, "NLOutput");
	//m_seq->push_back(register_module("linear2Output", torch::nn::Linear(hiddenLayerSize * 4, outputSize)));
}

torch::Tensor AttentionMultiImpl::forward(torch::Tensor x)
{
	return m_seq->forward(x);
}