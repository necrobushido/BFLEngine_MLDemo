#include "CrossAttentionMulti.h"

#include "SelfAttention.h"


//
CrossAttentionMultiBlockImpl::CrossAttentionMultiBlockImpl(int numHeads, int embedSize, int crossEmbedSize, double dropout, bool causal, eNonLinearity nonLinearity):
	m_attentionSeq(register_module("m_attentionSeq", torch::nn::Sequential())),
	m_nlSeq(register_module("m_nlSeq", torch::nn::Sequential())),
	m_crossAttentionLayerNorm(register_module("m_crossAttentionLayerNorm", torch::nn::LayerNorm(torch::nn::LayerNormOptions({embedSize})))),
	m_crossAttention(register_module("m_crossAttention", CrossAttention(numHeads, embedSize, crossEmbedSize, false, false, false))),
	m_crossDropout(register_module("m_crossDropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))))
{
	m_attentionSeq->push_back(register_module("layerNorm1", torch::nn::LayerNorm(torch::nn::LayerNormOptions({embedSize}))));
	m_attentionSeq->push_back(register_module("selfAttention", SelfAttention(numHeads, embedSize, false, false, causal)));
	m_attentionSeq->push_back(register_module("attentionDropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));

	m_nlSeq->push_back(register_module("layerNorm2", torch::nn::LayerNorm(torch::nn::LayerNormOptions({embedSize}))));
	m_nlSeq->push_back(register_module("linear1", torch::nn::Linear(embedSize, embedSize * 4)));
	AddNonLinearityModule(m_nlSeq, nonLinearity, "NL");
	m_nlSeq->push_back(register_module("linear2", torch::nn::Linear(embedSize * 4, embedSize)));
	m_nlSeq->push_back(register_module("nlDropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));
}

torch::Tensor CrossAttentionMultiBlockImpl::forward(torch::Tensor x, torch::Tensor crossValue)
{
	//torch::Tensor	longResidue = x;
	
	//	self attention + residual
	x = x + m_attentionSeq->forward(x);

	//	cross attention + residual
	torch::Tensor	crossResidue = x;
	x = m_crossAttentionLayerNorm->forward(x);
	x = m_crossAttention->forward(x, crossValue);
	x = m_crossDropout->forward(x);
	x = x + crossResidue;

	//	nonlinearity + residual
	x = x + m_nlSeq->forward(x);

	//	residual return
	//return x + longResidue;

	//	regular return
	return x;
}

//
CrossAttentionMultiImpl::CrossAttentionMultiImpl(int numHeads, int embedSize, int crossEmbedSize, int hiddenLayerCount, bool causal, double dropout, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential())),
	m_inputSeq(register_module("m_inputSeq", torch::nn::Sequential())),
	m_outputSeq(register_module("m_outputSeq", torch::nn::Sequential()))
{
	m_inputSeq->push_back("inputLinear", torch::nn::Identity());

	for(int i = 0; i < hiddenLayerCount; ++i)
	{
		char	nameBuffer[256];
		sprintf(nameBuffer, "AttentionMultiBlock%d", i);
		m_seq->push_back(register_module(nameBuffer, CrossAttentionMultiBlock(numHeads, embedSize, crossEmbedSize, dropout, causal, nonLinearity)));
	}

	m_outputSeq->push_back(register_module("layerNormOutput", torch::nn::LayerNorm(torch::nn::LayerNormOptions({embedSize}))));
	m_outputSeq->push_back("outputLinear", torch::nn::Identity());
}

CrossAttentionMultiImpl::CrossAttentionMultiImpl(int inputSize, int outputSize, int numHeads, int embedSize, int crossEmbedSize, int hiddenLayerCount, bool causal, double dropout, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential())),
	m_inputSeq(register_module("m_inputSeq", torch::nn::Sequential())),
	m_outputSeq(register_module("m_outputSeq", torch::nn::Sequential()))
{
	m_inputSeq->push_back("inputLinear", torch::nn::Linear(inputSize, embedSize));

	for(int i = 0; i < hiddenLayerCount; ++i)
	{
		char	nameBuffer[256];
		sprintf(nameBuffer, "AttentionMultiBlock%d", i);
		m_seq->push_back(register_module(nameBuffer, CrossAttentionMultiBlock(numHeads, embedSize, crossEmbedSize, dropout, causal, nonLinearity)));
	}

	m_outputSeq->push_back(register_module("layerNormOutput", torch::nn::LayerNorm(torch::nn::LayerNormOptions({embedSize}))));
	m_outputSeq->push_back("outputLinear", torch::nn::Linear(embedSize, outputSize));
}

torch::Tensor CrossAttentionMultiImpl::forward(torch::Tensor x, torch::Tensor crossValue)
{
	x = m_inputSeq->forward(x);

	for(int i = 0; i < m_seq->size(); ++i)
	{
		x = static_cast<CrossAttentionMultiBlockImpl*>((m_seq[i].get()))->forward(x, crossValue);
	}

	return m_outputSeq->forward(x);
}