#include "NNTextEncoder.h"

#include "AttentionMulti.h"
#include "NNTokenizer.h"

//
NNTextEncoderImpl*	g_NNTextEncoder = nullptr;

NNTextEncoderImpl::NNTextEncoderImpl():
	m_tokenEmbedding(register_module("m_tokenEmbedding", torch::nn::Embedding(g_NNTokenizer->GetVocabSize(), kEmbedSize))),
	m_positionEmbedding(register_parameter("m_positionEmbedding", torch::zeros({kMaxTokens, kEmbedSize}, torch::requires_grad()))),
	m_seq(register_module("m_seq", torch::nn::Sequential())),
	m_languageModelingHead(register_module("m_languageModelingHead", torch::nn::Linear(kEmbedSize, g_NNTokenizer->GetVocabSize())))
{
	Assert(g_NNTextEncoder == nullptr);
	g_NNTextEncoder = this;

	int		embedSize = kEmbedSize;
	int		numHeads = 12;
	int		numBlocks = 12;
	bool	causal = true;
	double	dropout = 0.2;
	//m_seq->push_back(register_module("attentionMulti", AttentionMulti(embedSize, embedSize, numHeads, embedSize, numBlocks, causal, dropout, kGelu)));
	m_seq->push_back(register_module("attentionMulti", AttentionMulti(numHeads, embedSize, numBlocks, causal, dropout, kNL_Gelu)));

	//	use the same weights for these
	m_tokenEmbedding->weight = m_languageModelingHead->weight;
}

NNTextEncoderImpl::~NNTextEncoderImpl()
{
	Assert(g_NNTextEncoder == this);
	g_NNTextEncoder = nullptr;
}

torch::Tensor NNTextEncoderImpl::forward(torch::Tensor inputTextTokenized)
{
	//	inputs 
	//	inputTextTokenized	: should be {batch, tokens}

	//	outputs
	//	should be {batch, tokens, embed}

	torch::Tensor	tokenEmbedding = m_tokenEmbedding->forward(inputTextTokenized);
	torch::Tensor	posEmbedding = m_positionEmbedding.slice(0, 0, inputTextTokenized.size(1));
	torch::Tensor	inputTextEmbedded = tokenEmbedding + posEmbedding;

	torch::Tensor	encodedText = m_seq->forward(inputTextEmbedded);

	return encodedText;
}

torch::Tensor NNTextEncoderImpl::LanguageModelingForward(torch::Tensor inputEncodedText)
{
	return m_languageModelingHead->forward(inputEncodedText);
}