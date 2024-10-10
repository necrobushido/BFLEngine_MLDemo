#include "NNCLIPComponent.h"

#include "MathNamespace.h"
#include "NNCLIP.h"
#include "NNVAE.h"

#include "AnimSeqDataSet.h"
#include "NNTokenizer.h"
#include "NNScheduling.h"

namespace
{
	enum
	{
		kCLIP_PromptText,
		kCLIP_AnimImage,

		kNumBatchComponents
	};

	const char*	kTensorLabels[] = 
	{
		"CLIP_PromptText",
		"CLIP_AnimImage"
	};

	enum
	{
		kPrintTrainingRate = 10
	};

	const bool	kAutoSaveOnBestValidation = true;
	const int	kValidationAutoSaveIterationThreshold = 700;

	enum
	{
		kSubBatchSize = 128,
		kSubBatchesPerBatch = 1,
		kBatchSize = kSubBatchSize * kSubBatchesPerBatch
	};
}

NNCLIPComponent::NNCLIPComponent():
	m_pNNCLIP(nullptr),
	m_pDefaultOptimizer(nullptr),
	m_pGoBOptimizer(nullptr),
	m_pNonGoBOptimizer(nullptr),
	m_deviceToUse(c10::DeviceType::CPU),
	//m_learningRate(7.0e-05f),
	m_learningRate(0.0005f),
	//m_learningRate(5.0e-6f),
	m_iterationCount(0),
	m_pScheduler(nullptr),
	m_accumSubBatchLoss(0.0f),
	m_currentSubBatch(0)
{
	m_epochGen.Init();

	m_pNNCLIP = new NNCLIP();

	int64_t	parameterTensorCount;
	int64_t	parameterElementCount;
	GetParameterCount(*(*m_pNNCLIP).get(), &parameterTensorCount, &parameterElementCount);
	DebugPrintf("NNCLIPComponent parameter tensor count = %lld, element count = %lld\n", parameterTensorCount, parameterElementCount);

	(*m_pNNCLIP)->apply(CommonInitModule);
}

NNCLIPComponent::~NNCLIPComponent()
{
	delete m_pScheduler;
	delete m_pNonGoBOptimizer;
	delete m_pGoBOptimizer;
	delete m_pDefaultOptimizer;
	delete m_pNNCLIP;
}

void NNCLIPComponent::CalcLoss(torch::Tensor promptText, torch::Tensor latentAnimData, torch::Tensor* pLossOut, std::string* pLossLogOut)
{
	int				batchSize = (int)latentAnimData.size(0);

	//	project to shared embed space
	torch::Tensor	encodedText = (*m_pNNCLIP)->ProjectTextToShared(promptText);
	torch::Tensor	encodedAnim = (*m_pNNCLIP)->ProjectAnimToShared(latentAnimData);

	//	cosine similarity as logits
	torch::Tensor	logitScale = (*m_pNNCLIP)->GetLNLogitScale().exp();
	torch::Tensor	logitsPerAnim = encodedAnim.matmul(encodedText.t()) * logitScale;
	//torch::Tensor	logitsPerAnim = encodedAnim.matmul(encodedText.t()) * 14.2857;
	//torch::Tensor	logitsPerAnim = encodedAnim.matmul(encodedText.t());
	torch::Tensor	logitsPerText = logitsPerAnim.t();

	torch::Tensor	groundTruth = torch::arange(batchSize, torch::kLong).to(m_deviceToUse);
	torch::Tensor	lossAnim = torch::nn::functional::cross_entropy(logitsPerAnim, groundTruth);
	torch::Tensor	lossText = torch::nn::functional::cross_entropy(logitsPerText, groundTruth);

	torch::Tensor	crossEntropyLoss = ((lossAnim + lossText) * 0.5);

	//	is this redundant?
	/*torch::Tensor	logitsPerAnimUnscaled = encodedAnim.matmul(encodedText.t());
	torch::Tensor	identityMtx = torch::eye(batchSize, m_deviceToUse);
	torch::Tensor	mseLoss = torch::nn::functional::mse_loss(logitsPerAnimUnscaled, identityMtx);*/

	//	is this redundant?
	//		I guess it lets me weight this more heavily if I want to
	torch::Tensor	alignmentLoss = DotProductAlignmentLoss(encodedText, encodedAnim, 1.0f, false);
	alignmentLoss = torch::sum(alignmentLoss) / alignmentLoss.size(0);

	//
	*pLossOut = crossEntropyLoss;
	//*pLossOut = crossEntropyLoss + alignmentLoss * 10.0;
	//*pLossOut = crossEntropyLoss + mseLoss + alignmentLoss;

	//	log
	{
		char	tempBuffer[256];
		sprintf(tempBuffer, "lossAnim = %.9g, lossText = %.9g, alignmentLoss = %.9g, logitScale = %.9g", lossAnim.item().toFloat(), lossText.item().toFloat(), alignmentLoss.item().toFloat(), logitScale.item().toFloat());

		*pLossLogOut = tempBuffer;
	}
}

void NNCLIPComponent::TrainingIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap, bool shouldPrint)
{
	torch::Tensor	loss;
	torch::Tensor	promptText = tagToBatchTensorMap[kTensorLabels[kCLIP_PromptText]];
	torch::Tensor	animImage = tagToBatchTensorMap[kTensorLabels[kCLIP_AnimImage]];

	torch::Tensor	logVar;
	torch::Tensor	mean;
	torch::Tensor	latentAnimData;

	std::string		lossLog;

	{
		torch::NoGradGuard	no_grad;

		latentAnimData = g_NNVAE->EncoderForward(animImage, &logVar, &mean);
	}

	//CalcLoss(promptText, animImage, &loss);
	CalcLoss(promptText, latentAnimData, &loss, &lossLog);

	if( m_currentSubBatch == 0 )
	{
		bool				set_to_none = true;
		if( m_pDefaultOptimizer )
		{
			m_pDefaultOptimizer->zero_grad(set_to_none);
		}
		if( m_pGoBOptimizer )
		{
			m_pGoBOptimizer->zero_grad(set_to_none);
		}
		if( m_pNonGoBOptimizer )
		{
			m_pNonGoBOptimizer->zero_grad(set_to_none);
		}

		m_accumSubBatchLoss = 0.0f;
	}

	loss = loss * (1.0f / (float)kSubBatchesPerBatch);	//	scale the loss down so that we average sub-batch gradients correctly
	loss.backward();

	m_currentSubBatch++;

	m_accumSubBatchLoss += loss.item().toFloat();

	if( m_currentSubBatch >= kSubBatchesPerBatch )
	{
		m_currentSubBatch = 0;

		double	gradNorm = torch::nn::utils::clip_grad_norm_((*m_pNNCLIP)->parameters(), 2000.0);	//	just so that I can print it atm
		//double	gradNorm = torch::nn::utils::clip_grad_norm_((*m_pNNCLIP)->parameters(), 1.0);
		if( m_pDefaultOptimizer )
		{
			m_pDefaultOptimizer->step();
		}
		if( m_pGoBOptimizer )
		{
			m_pGoBOptimizer->step();
		}
		if( m_pNonGoBOptimizer )
		{
			m_pNonGoBOptimizer->step();
		}

		m_trainingLossHistory.push_back(m_accumSubBatchLoss);

		int	currentEpoch = m_epochGen.GetCreationCycles();

		if( shouldPrint &&
			(m_iterationCount % kPrintTrainingRate == 0) )
		{
			f32	recentLossAvg = 0;
			int	avgThreshold = 1000;
			int	itemCount = std::min(avgThreshold, (int)m_trainingLossHistory.size());
			int	startLoop = (int)m_trainingLossHistory.size()-1;
			int	endLoop = startLoop - itemCount;
			for(int i = startLoop; i > endLoop; --i)
			{
				recentLossAvg += m_trainingLossHistory[i];
			}
			recentLossAvg /= itemCount;

		/*	f32	recentVariance = 0;
			for(int i = startLoop; i > endLoop; --i)
			{
				f32		thisSE = (m_trainingLossHistory[i] - recentLossAvg);
				thisSE = thisSE * thisSE;
				recentVariance += thisSE;
			}
			recentVariance /= itemCount;

			f32	recentStdDev = sqrt(recentVariance);*/

			DebugPrintf("Train NNCLIPComponent %d : current loss = %.9g, recent loss avg = %.9g, epoch = %d, gradNorm = %.9g\n", m_iterationCount, m_accumSubBatchLoss, recentLossAvg, currentEpoch, gradNorm);
			DebugPrintf("\t%s\n", lossLog.c_str());
			DebugPrintf("\n");
		}

		m_iterationCount++;
		UpdateOptimizer();
	}
}

void NNCLIPComponent::ValidationIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap)
{
	torch::NoGradGuard	no_grad;

	torch::Tensor	loss;
	torch::Tensor	promptText = tagToBatchTensorMap[kTensorLabels[kCLIP_PromptText]];
	torch::Tensor	animImage = tagToBatchTensorMap[kTensorLabels[kCLIP_AnimImage]];

	torch::Tensor	logVar;
	torch::Tensor	mean;
	torch::Tensor	latentAnimData;

	std::string		lossLog;


	{
		torch::NoGradGuard	no_grad;

		latentAnimData = g_NNVAE->EncoderForward(animImage, &logVar, &mean);
	}

	//CalcLoss(promptText, animImage, &loss);
	CalcLoss(promptText, latentAnimData, &loss, &lossLog);

	{
		f32			currentLoss = loss.item().toFloat();
		m_validationLossHistory.push_back(currentLoss);

		f32	recentLossAvg = 0;
		int	avgThreshold = 10;
		int	itemCount = std::min(avgThreshold, (int)m_validationLossHistory.size());
		int	startLoop = (int)m_validationLossHistory.size()-1;
		int	endLoop = startLoop - itemCount;
		for(int i = startLoop; i > endLoop; --i)
		{
			recentLossAvg += m_validationLossHistory[i];
		}
		recentLossAvg /= itemCount;

		/*f32	recentVariance = 0;
		for(int i = startLoop; i > endLoop; --i)
		{
			f32		thisSE = (m_validationLossHistory[i] - recentLossAvg);
			thisSE = thisSE * thisSE;
			recentVariance += thisSE;
		}
		recentVariance /= itemCount;

		f32	recentStdDev = sqrt(recentVariance);*/

		static f32		bestValidationAvg = 99999999999999999.0f;
		if( recentLossAvg < bestValidationAvg )
		{
			bestValidationAvg = recentLossAvg;

			if( kAutoSaveOnBestValidation &&
				m_iterationCount > kValidationAutoSaveIterationThreshold )
			{
				Save();
				(*m_pNNCLIP)->train();	//	Save sets the module to eval mode (does it need to?  I should test), so switch back

				DebugPrintf("New best validation saved.\n\n");
			}
		}

		float	lrMult = m_pScheduler->Schedule(m_iterationCount);

		DebugPrintf("VALIDATION NNCLIPComponent : current loss = %.9g, recent loss avg = %.9g\n", currentLoss, recentLossAvg);
		DebugPrintf("\t%s\n", lossLog.c_str());
		DebugPrintf("\tbase learning rate = %.9g, effective learning rate = %.9g, best validation avg seen = %.9g\n", m_learningRate, m_learningRate * lrMult, bestValidationAvg);
		DebugPrintf("\n");
	}
}

void NNCLIPComponent::Save()
{
	SaveModule(m_pNNCLIP->get(), "NNCLIPComponent.pt");
}

void NNCLIPComponent::Load()
{
	LoadModule(m_pNNCLIP->get(), "NNCLIPComponent.pt");
}

void NNCLIPComponent::TrainMode()
{
	(*m_pNNCLIP)->train();
}

void NNCLIPComponent::EvalMode()
{
	(*m_pNNCLIP)->eval();
}

int NNCLIPComponent::SubBatchSize()
{
	return kSubBatchSize;
}

int NNCLIPComponent::SubBatchesPerBatch()
{
	return kSubBatchesPerBatch;
}

void NNCLIPComponent::UpdateOptimizer()
{
	if( m_pDefaultOptimizer || m_pGoBOptimizer || m_pNonGoBOptimizer )
	{
		float	lrMult = m_pScheduler->Schedule(m_iterationCount);
		//float	lrMult = 1.0f;

		if( m_pNonGoBOptimizer )
		{
			std::vector<torch::optim::OptimizerParamGroup>&	paramGroups = m_pNonGoBOptimizer->param_groups();
			for(std::vector<torch::optim::OptimizerParamGroup>::iterator iter = paramGroups.begin(); iter != paramGroups.end(); iter++)
			{
				iter->options().set_lr(m_learningRate * lrMult);
			}
		}

		if( m_pGoBOptimizer )
		{
			std::vector<torch::optim::OptimizerParamGroup>&	paramGroups = m_pGoBOptimizer->param_groups();
			for(std::vector<torch::optim::OptimizerParamGroup>::iterator iter = paramGroups.begin(); iter != paramGroups.end(); iter++)
			{
				iter->options().set_lr(m_learningRate * lrMult);
			}
		}

		if( m_pDefaultOptimizer )
		{
			std::vector<torch::optim::OptimizerParamGroup>&	paramGroups = m_pDefaultOptimizer->param_groups();
			for(std::vector<torch::optim::OptimizerParamGroup>::iterator iter = paramGroups.begin(); iter != paramGroups.end(); iter++)
			{
				iter->options().set_lr(m_learningRate * lrMult);
			}
		}
	}
	else
	{
		if( false )
		{
			std::vector<torch::Tensor>	gainOrBiasParams;
			std::vector<torch::Tensor>	nonGainOrBiasParams;
			(*m_pNNCLIP)->GetGainOrBiasParams(&gainOrBiasParams, &nonGainOrBiasParams);

			torch::optim::AdamWOptions	adamwOptions(m_learningRate);
			adamwOptions.weight_decay() = 0.2;
			adamwOptions.betas({0.9, 0.95});
			adamwOptions.eps() = 1e-8;
			m_pNonGoBOptimizer = new torch::optim::AdamW(nonGainOrBiasParams, adamwOptions);

			adamwOptions.weight_decay() = 0.0;
			m_pGoBOptimizer = new torch::optim::AdamW(gainOrBiasParams, adamwOptions);
		}
		else
		{
			torch::optim::AdamWOptions	adamwOptions(m_learningRate);
			adamwOptions.weight_decay() = 0.1;
			adamwOptions.betas({0.9, 0.95});
			adamwOptions.eps() = 1e-8;
			m_pDefaultOptimizer = new torch::optim::AdamW((*m_pNNCLIP)->parameters(true), adamwOptions);
		}

		//
		/*std::vector<int64_t>	warmUpSteps = {1000};
		std::vector<float>		LRMultMin = {0.1f};
		std::vector<float>		LRMultMax = {1.0f};
		std::vector<float>		LRMultWarmUpStart = {1.0e-6f};
		std::vector<int64_t>	cycleLength = {10000};*/

		std::vector<int64_t>	warmUpSteps = {1000, 100};
		std::vector<float>		LRMultMin = {0.1f, 0.1f};
		std::vector<float>		LRMultMax = {1.0f, 0.1f};
		std::vector<float>		LRMultWarmUpStart = {1.0e-6f, 0.1f};
		std::vector<int64_t>	cycleLength = {10000, 10000000000};

		//
		//m_pScheduler = new NNLambdaLinearScheduler(warmUpSteps, LRMultMin, LRMultMax, LRMultWarmUpStart, cycleLength);
		m_pScheduler = new NNLambdaWarmUpCosineScheduler2(warmUpSteps, LRMultMin, LRMultMax, LRMultWarmUpStart, cycleLength);
		//m_pScheduler = new NNLambdaWarmUpCosineScheduler(10000, 0.1f, 1.0f, 1.0e-6f, 10000);
		//m_pScheduler = new NNLambdaWarmUpCosinePeriodScheduler(10000, 0.1f, 1.0f, 1.0e-6f, 10000);
		//m_pScheduler = new NNLambdaWarmUpCosineAnnealingScheduler(0.1f, 1.0f, 1000, 10000, 0.5f);
		//m_pScheduler = new NNLambdaWarmUpCosineAnnealingScheduler(0.01f, 0.1f, 1000, 10000, 0.5f);
		//m_pScheduler = new NNScheduler();
	}
}

void NNCLIPComponent::SetLearningRate(f32 learningRate)
{
	m_learningRate = learningRate;
}

void NNCLIPComponent::SetDevice(torch::Device device)
{
	m_deviceToUse = device;

	(*m_pNNCLIP)->to(m_deviceToUse);
}

const std::vector<f32>* NNCLIPComponent::GetLossHistory()
{
	return &m_trainingLossHistory;
}

void NNCLIPComponent::ResetLossHistory()
{
	m_trainingLossHistory.clear();
	m_validationLossHistory.clear();
}

bool NNCLIPComponent::IsTraining()
{
	return (*m_pNNCLIP)->is_training();
}

void NNCLIPComponent::GetBatch(int batchSize, eBatchType batchType, std::map<std::string, torch::Tensor>* tagToBatchTensorMap)
{
	torch::Tensor	dataTensor = (batchType == kBatchType_Validation) ? (g_animSeqDataSet->m_validationText) : (g_animSeqDataSet->m_trainingText);

	std::vector<AnimationIntermediateSeq>*	pBatchAnims = (batchType == kBatchType_Validation) ? (&g_animSeqDataSet->m_validationAnims) : (&g_animSeqDataSet->m_trainingAnims);

	Assert(g_NNTokenizer != nullptr);

	int				startToken = g_NNTokenizer->GetStartToken();
	int				endToken = g_NNTokenizer->GetEndToken();

	torch::Tensor	startTokenTensor = torch::tensor({startToken}).to(torch::kInt64);
	torch::Tensor	endTokenTensor = torch::tensor({endToken}).to(torch::kInt64);

	std::vector<torch::Tensor>	tensorLists[kNumBatchComponents];
	for(int batchItemIdx = 0; batchItemIdx < batchSize; ++batchItemIdx)
	{
		int				animIdx = 0;
		if( batchType == kBatchType_Validation )
		{
			//animIdx = rand() % pBatchAnims->size();
			animIdx = batchItemIdx % pBatchAnims->size();	//	best not to have dupes in the validation test
		}
		else
		{
			NNAnimEpoch::BatchItemData	batchItemData = m_epochGen.PopNextBatchItem();
			animIdx = batchItemData.m_animIdx;
		}

		AnimationIntermediateSeq*	sourceAnim = &(*pBatchAnims)[animIdx];

		//	prompt
		{
			std::string		prompt = sourceAnim->descString;
			torch::Tensor	animTextDescTokenized = g_NNTokenizer->EncodePaddedTensor(prompt, NNCLIPImpl::kMaxTokens, true, true);

			tensorLists[kCLIP_PromptText].push_back(animTextDescTokenized);
		}

		//	anim
		{			
			torch::Tensor	wholeAnimImageLocal = sourceAnim->animImageC3DR6.GetLocalKeyframeImageForWholeAnim();
			tensorLists[kCLIP_AnimImage].push_back(wholeAnimImageLocal);
		}
	}

	for(int batchComponentIdx = 0; batchComponentIdx < kNumBatchComponents; ++batchComponentIdx)
	{
		(*tagToBatchTensorMap)[kTensorLabels[batchComponentIdx]] = torch::stack(tensorLists[batchComponentIdx]);
	}
}