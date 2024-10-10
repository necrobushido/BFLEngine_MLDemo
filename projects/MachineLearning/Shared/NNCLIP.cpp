#include "NNCLIP.h"

#include "AttentionMulti.h"
#include "NNTokenizer.h"
#include "NNUtilityBlocks.h"

#include "AnimGenBoneDictionary.h"
#include "MathNamespace.h"

#include "Encoder2D.h"

namespace
{
	//	source was also looking for batchNorm; might want to add any other norms that come up?
	const char*	kGoBParamNames[] =
	{
		//"layerNorm",
		//"groupNorm",
		"Norm",
		"logitScale",
		"bias"
	};

	enum
	{
		kGoBParamNameCount = ARRAY_SIZE(kGoBParamNames)
	};

	const double	kDropout = 0.1;
}

//
NNCLIPProjHeadImpl::NNCLIPProjHeadImpl(int inputSize, double dropout):
	m_projSeq(register_module("m_projSeq", torch::nn::Sequential())),
	m_nlSeq(register_module("m_nlSeq", torch::nn::Sequential()))
{
	//m_projSeq->push_back(register_module("layerNorm", torch::nn::LayerNorm(torch::nn::LayerNormOptions({inputSize}))));
	m_projSeq->push_back(register_module("linear1", torch::nn::Linear(inputSize, NNCLIPImpl::kSharedEmbedSize)));

	AddNonLinearityModule(m_nlSeq, kNL_Gelu, "NL");
	m_nlSeq->push_back(register_module("linear2", torch::nn::Linear(NNCLIPImpl::kSharedEmbedSize, NNCLIPImpl::kSharedEmbedSize)));
	m_nlSeq->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));

	//AddNonLinearityModule(m_projSeq, kNL_Gelu, "NL");
	//m_projSeq->push_back(register_module("linear2", torch::nn::Linear(NNCLIPImpl::kSharedEmbedSize, NNCLIPImpl::kSharedEmbedSize)));
}

NNCLIPProjHeadImpl::~NNCLIPProjHeadImpl()
{
}

torch::Tensor NNCLIPProjHeadImpl::forward(torch::Tensor x)
{
	x = m_projSeq->forward(x);
	x = x + m_nlSeq->forward(x);
	//x = m_nlSeq->forward(x);

	return x;
}

/*
best test so far:
64 tokens and simple projection, 8 attention blocks, 1024 text embed
NNProcessingThreadBase::ValidationIteration : 20.061737 minutes, 2499 training iterations
New best validation saved.

VALIDATION NNCLIPComponent : current loss = 1.28330123, recent loss avg = 1.28122425
	lossAnim = 1.27316308, lossText = 1.29343939
	base learning rate = 0.000500000024, effective learning rate = 0.000469894992, best validation avg seen = 1.28122425

Train NNCLIPComponent 2500 : current loss = 0.167737678, recent loss avg = 0.219585314, epoch = 139, gradNorm = 2.82621217, logitScale = 3.32456446
	lossAnim = 0.164396986, lossText = 0.171078369
*/

//
NNCLIPImpl*	g_NNCLIP = nullptr;

NNCLIPImpl::NNCLIPImpl():
	m_tokenEmbedding(register_module("m_tokenEmbedding", torch::nn::Embedding(Math::NextPowerOf2(g_NNTokenizer->GetVocabSize()), kTextEmbedSize))),
	m_positionEmbedding(register_parameter("m_positionEmbedding", torch::empty({kMaxTokens, kTextEmbedSize}, torch::requires_grad()))),
	m_textTransformerSeq(register_module("m_textTransformerSeq", torch::nn::Sequential())),
	m_animEncoderSeq(register_module("m_animEncoderSeq", torch::nn::Sequential())),
	m_lnLogitScale(register_parameter("m_lnLogitScale", torch::tensor({Math::LN(1.0f / 0.07f)}, torch::requires_grad()))),
	//m_textToSharedProj(register_module("m_textToSharedProj", NNCLIPProjHead(kTextEmbedSize, kDropout))),
	//m_animToSharedProj(register_module("m_animToSharedProj", NNCLIPProjHead(kAnimEmbedSize, kDropout))),
	m_textProjection(register_parameter("m_textProjection", torch::empty({kTextEmbedSize, kSharedEmbedSize}, torch::requires_grad())))
{
	Assert(g_NNCLIP == nullptr);
	g_NNCLIP = this;

	int		embedSize = kTextEmbedSize;	//	increasing this does help get validation lower (more than blocks)
	int		numHeads = embedSize / 64;
	int		numBlocks = 8;				//	increasing this does help get validation lower (worse than embed)
	bool	textCausal = true;
	m_textTransformerSeq->push_back(register_module("textAttentionMulti", AttentionMulti(numHeads, embedSize, numBlocks, textCausal, kDropout, kNL_Gelu)));

	//
	/*
		{b, 16, 32, 32}
		conv down to {b, z, 32, 32}?
	*/
	{		
		//m_animEncoderSeq->push_back(register_module("animRes11", NNResidualBlock2D(16, 8, 1, kDropout, kNL_Silu)));
		///m_animEncoderSeq->push_back(register_module("animRes21", NNResidualBlock2D(8, 4, 1, kDropout, kNL_Silu)));
		//m_animEncoderSeq->push_back(register_module("animRes31", NNResidualBlock2D(4, 2, 1, kDropout, kNL_Silu)));
		//m_animEncoderSeq->push_back(register_module("animRes41", NNResidualBlock2D(2, 1, 1, kDropout, kNL_Silu)));

		//m_animEncoderSeq->push_back(register_module("animGroupNorm", torch::nn::GroupNorm(4, 16)));
		//AddNonLinearityModule(m_animEncoderSeq, kNL_Silu, "NL");
		//m_animEncoderSeq->push_back(register_module("animConvDown", torch::nn::Conv2d(torch::nn::Conv2dOptions(16, 4, 3).padding(1))));

		m_animEncoderSeq->push_back(register_module("animConvDown", NNResidualBlock2D(16, 4, 1, kDropout, kNL_Silu)));

		/*{
			std::vector<int>	channelMult = {1, 2, 4, 4, 4, 4};
			int					numResBlocksPerEntry = 2;

			int					inputSize = 16;
			int					outputSize = 512;
			int					hiddenSize = 128;
			int					normGroups = 32;

			m_animEncoderSeq->push_back(register_module("animEncoder", Encoder2D(inputSize, outputSize, hiddenSize, normGroups, kDropout, channelMult, numResBlocksPerEntry, kNL_Silu)));
		}*/
	}

	//	do specific initialization of params
	torch::nn::init::normal_(m_positionEmbedding, 0.0, 0.01);
	torch::nn::init::normal_(m_textProjection, 0.0, 1.0 / sqrt((double)kTextEmbedSize));
}

NNCLIPImpl::~NNCLIPImpl()
{
	Assert(g_NNCLIP == this);
	g_NNCLIP = nullptr;
}

torch::Tensor NNCLIPImpl::forward(torch::Tensor inputTextTokenized)
{
	return EncodeText(inputTextTokenized);
}

torch::Tensor NNCLIPImpl::EncodeText(torch::Tensor inputTextTokenized)
{
	torch::Tensor	tokenEmbedding = m_tokenEmbedding->forward(inputTextTokenized);
	torch::Tensor	posEmbedding = m_positionEmbedding.slice(0, 0, inputTextTokenized.size(1));
	torch::Tensor	inputTextEmbedded = tokenEmbedding + posEmbedding;

	torch::Tensor	encodedText = m_textTransformerSeq->forward(inputTextEmbedded);

	return encodedText;
}

torch::Tensor NNCLIPImpl::EncodeAnim(torch::Tensor inputAnimLatent)
{
	torch::Tensor	encodedAnim = m_animEncoderSeq->forward(inputAnimLatent);
	return encodedAnim;

	//return inputAnimLatent;
}

torch::Tensor NNCLIPImpl::ProjectTextToShared(torch::Tensor text)
{
	//	only use the embedding of the last token
	//torch::Tensor	encodedText = EncodeText(text).slice(1, -1);

	//	only use the embedding of where the original text ended (ignore padding)
	torch::Tensor	batchIdx = torch::arange(text.size(0), text.device());	
	torch::Tensor	textArgMax = text.argmax(-1);	//	the "end" token should be the largest token
	torch::Tensor	encodedText = EncodeText(text);
	encodedText = encodedText.index({batchIdx, textArgMax});

	//	vectorize encoding
	encodedText = encodedText.view({encodedText.size(0), -1});

	//	project to shared embed space
	//encodedText = m_textToSharedProj->forward(encodedText);
	encodedText = encodedText.matmul(m_textProjection);

	//	normalize
	encodedText = encodedText / torch::norm(encodedText, 2, -1, true);

	return encodedText;
}

torch::Tensor NNCLIPImpl::ProjectAnimToShared(torch::Tensor anim)
{
	torch::Tensor	encodedAnim = EncodeAnim(anim);

	//	vectorize encoding
	encodedAnim = encodedAnim.view({encodedAnim.size(0), -1});

	//	project to shared embed space
	//encodedAnim = m_animToSharedProj->forward(encodedAnim);

	//	normalize
	encodedAnim = encodedAnim / torch::norm(encodedAnim, 2, -1, true);

	return encodedAnim;
}

void NNCLIPImpl::GetGainOrBiasParams(std::vector<torch::Tensor>* gainOrBiasParamsOut, std::vector<torch::Tensor>* nonGainOrBiasParamsOut)
{
	//	is this actually correct?  there might be some params in SelfAttention that we're missing?
	torch::OrderedDict<std::string, torch::Tensor>	allNamedParams = named_parameters(true);
	for(int i = 0; i < (int)allNamedParams.size(); ++i)
	{
		bool		isGoB = false;

		if( allNamedParams[i].value().sizes().size() < 2 )
		{
			gainOrBiasParamsOut->push_back(allNamedParams[i].value());
			isGoB = true;
		}

		std::string	paramName = allNamedParams[i].key();
		for(int subStrIdx = 0; subStrIdx < kGoBParamNameCount && !isGoB; ++subStrIdx)
		{
			if( paramName.find(kGoBParamNames[subStrIdx]) != std::string::npos) 
			{
				gainOrBiasParamsOut->push_back(allNamedParams[i].value());
				isGoB = true;
			}			
		}

		if( !isGoB )
		{
			nonGainOrBiasParamsOut->push_back(allNamedParams[i].value());
		}
	}
}