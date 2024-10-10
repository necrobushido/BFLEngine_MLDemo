#include "NNVAEModelComponent.h"

#include "MathNamespace.h"

#include "AnimImage.h"
#include "NNVAE.h"

#include "AnimSeqDataSet.h"

namespace
{
	enum
	{
		kAnimImage,
		//kAnimBones,
		//kTrainingAnimIdx,
		//kTargetSampleKeyframes,
		//kTargetSampleTimes,

		kNumBatchComponents
	};

	const char*	kTensorLabels[] = 
	{
		"VAE_AnimImage",
		//"VAE_AnimBones",
		//"VAE_TrainingAnimIdx",
		//"VAE_TargetSampleKeyframes",
		//"VAE_TargetSampleTime"
	};
}

NNVAEModelComponent::NNVAEModelComponent():
	m_pNNVAE(nullptr),
	m_pOptimizer(nullptr),
	m_deviceToUse(c10::DeviceType::CPU),
	m_learningRate(4.5e-6f),
	m_requestTrainingEnd(false)
{
	//	C3DR6
	m_pNNVAE = new NNVAE(Continuous3DRotation6::kElementCount);

	//	Euler
	//m_pNNVAE = new NNVAE(Vector3::kElementCount);

	int64_t	parameterTensorCount;
	int64_t	parameterElementCount;
	GetParameterCount(*(*m_pNNVAE).get(), &parameterTensorCount, &parameterElementCount);
	DebugPrintf("NNVAE parameter tensor count = %lld, element count = %lld\n", parameterTensorCount, parameterElementCount);

	(*m_pNNVAE)->apply(CommonInitModule);
}

NNVAEModelComponent::~NNVAEModelComponent()
{
	delete m_pOptimizer;
	delete m_pNNVAE;
}

void NNVAEModelComponent::CalcLoss(torch::Tensor inputAnimData, torch::Tensor& lossOut, std::string& logOut)
{
	torch::Tensor		logVar;
	torch::Tensor		mean;
	torch::Tensor		encodedData = (*m_pNNVAE)->EncoderForward(inputAnimData, &logVar, &mean);
	torch::Tensor		decodedData = (*m_pNNVAE)->DecoderForward(encodedData);

	(*m_pNNVAE)->CalcLoss(inputAnimData, decodedData, mean, logVar, lossOut, logOut);
}

void NNVAEModelComponent::TrainingIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap, bool shouldPrint)
{
	torch::Tensor	loss;
	std::string		lossLog;

	torch::Tensor	inputAnimData = tagToBatchTensorMap[kTensorLabels[kAnimImage]];
	
	CalcLoss(inputAnimData, loss, lossLog);

	//	gradient descent
	loss.backward();
	//torch::nn::utils::clip_grad_value_((*m_pNNVAE)->parameters(), 1.0);	//	clip gradients to 1
	//torch::nn::utils::clip_grad_norm_((*m_pNNVAE)->parameters(), 2.0);	//	clip gradients to max norm 2
	m_pOptimizer->step();
	bool		set_to_none = true;	//	I guess this tosses the gradient instead of memsetting it to zero?
	m_pOptimizer->zero_grad(set_to_none);

	f32			currentLoss = loss.item().toFloat();
	m_trainingLossHistory.push_back(currentLoss);

	static int	iterationCount = 0;
	iterationCount++;

	if( shouldPrint )
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

		f32	recentVariance = 0;
		for(int i = startLoop; i > endLoop; --i)
		{
			f32		thisSE = (m_trainingLossHistory[i] - recentLossAvg);
			thisSE = thisSE * thisSE;
			recentVariance += thisSE;
		}
		recentVariance /= itemCount;

		f32	recentStdDev = sqrt(recentVariance);

		DebugPrintf("Train NNVAEModelComponent %d : current loss = %.9g, recent loss avg = %.9g, recent loss stddev = %.9g\n", iterationCount, currentLoss, recentLossAvg, recentStdDev);
		DebugPrintf("\t%s\n", lossLog.c_str());
		DebugPrintf("\n");
	}
}

void NNVAEModelComponent::ValidationIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap)
{
	torch::NoGradGuard	no_grad;

	torch::Tensor		loss;
	std::string			lossLog;

	torch::Tensor		inputAnimData = tagToBatchTensorMap[kTensorLabels[kAnimImage]];

	CalcLoss(inputAnimData, loss, lossLog);

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

		f32	recentVariance = 0;
		for(int i = startLoop; i > endLoop; --i)
		{
			f32		thisSE = (m_validationLossHistory[i] - recentLossAvg);
			thisSE = thisSE * thisSE;
			recentVariance += thisSE;
		}
		recentVariance /= itemCount;

		f32	recentStdDev = sqrt(recentVariance);

		static f32	bestValidationAvg = 99999999999999999.0f;
		if( recentLossAvg < bestValidationAvg )
		{
			bestValidationAvg = recentLossAvg;
		}

		DebugPrintf("VALIDATION NNVAEModelComponent : current loss = %.9g, recent loss avg = %.9g, recent loss stddev = %.9g\n", currentLoss, recentLossAvg, recentStdDev);
		DebugPrintf("\t%s\n", lossLog.c_str());
		DebugPrintf("\tcurrent learning rate = %.9g, best validation avg seen = %.9g\n", m_learningRate, bestValidationAvg);
		DebugPrintf("\n");


		//		
		const float		kLearningRateLowerBound = 1.0e-9f;

		//	method based on stagnant recent validation performance
		//const int		kBadValidationCountThreshold = 4;
		//const float		kCurrentLossThresholdScale = 1.1f;		
		//static int		s_recentWorseThanAverageValidationLoss = 0;

		////	should prob change this to something like
		////if( currentLoss - recentLossAvg > -(recentStdDev * 0.5) )	//	dunno what portion of the stdDev should count as bad
		//if( (currentLoss * kCurrentLossThresholdScale) > recentLossAvg )
		//{
		//	s_recentWorseThanAverageValidationLoss++;
		//	if( s_recentWorseThanAverageValidationLoss > kBadValidationCountThreshold )
		//	{
		//		if( m_learningRate > kLearningRateLowerBound )
		//		{
		//			float	newLearningRate = m_learningRate * 0.5f;
		//			SetLearningRate(newLearningRate);
		//			UpdateOptimizer();
		//			DebugPrintf("Lowering learning rate to = %.9g\n\n", newLearningRate);
		//		}
		//		else
		//		{
		//			DebugPrintf("Would lower learning rate, but stopped by lower bound of %.9g\n\n", kLearningRateLowerBound);
		//			m_requestTrainingEnd = true;
		//		}

		//		s_recentWorseThanAverageValidationLoss = 0;
		//	}
		//}
		//else
		//{
		//	s_recentWorseThanAverageValidationLoss = 0;
		//}

		//	method based on how long it has been since the best validation avg was recorded
		const int		kBestValidationAvgThreshold = 10;
		static float	s_bestValidationLossAvg = 99999999999999999.0f;
		static float	s_iterationsSinceBestValidationLossAvg = 0;
		if( recentLossAvg < s_bestValidationLossAvg )
		{
			s_bestValidationLossAvg = recentLossAvg;
			s_iterationsSinceBestValidationLossAvg = 0;
		}
		else
		{
			s_iterationsSinceBestValidationLossAvg++;
			if( s_iterationsSinceBestValidationLossAvg > kBestValidationAvgThreshold )
			{
				if( m_learningRate > kLearningRateLowerBound )
				{
					float	newLearningRate = m_learningRate * 0.5f;
					SetLearningRate(newLearningRate);
					UpdateOptimizer();
					DebugPrintf("Lowering learning rate to = %.9g\n\n", newLearningRate);
				}
				else
				{
					DebugPrintf("Would lower learning rate, but stopped by lower bound of %.9g\n\n", kLearningRateLowerBound);
					m_requestTrainingEnd = true;
				}

				//	reset the counter and hope things get better with the lower learning rate
				s_iterationsSinceBestValidationLossAvg = 0;
			}
		}

		//	nan check
		if( currentLoss != currentLoss )
		{
			DebugPrintf("NaN loss detected.  Stopping training.\n\n");
			m_requestTrainingEnd = true;
		}
	}
}

void NNVAEModelComponent::Save()
{
	SaveModule(m_pNNVAE->get(), "NNVAE.pt");
}

void NNVAEModelComponent::Load()
{
	LoadModule(m_pNNVAE->get(), "NNVAE.pt");
}

void NNVAEModelComponent::TrainMode()
{
	m_requestTrainingEnd = false;
	(*m_pNNVAE)->train();

	std::vector<torch::Tensor>	modelParams = (*m_pNNVAE)->parameters(true);
	for(std::vector<torch::Tensor>::iterator iter = modelParams.begin(); iter != modelParams.end(); iter++)
	{
		iter->set_requires_grad(true);
	}
}

void NNVAEModelComponent::EvalMode()
{
	(*m_pNNVAE)->eval();

	std::vector<torch::Tensor>	modelParams = (*m_pNNVAE)->parameters(true);
	for(std::vector<torch::Tensor>::iterator iter = modelParams.begin(); iter != modelParams.end(); iter++)
	{
		iter->set_requires_grad(false);
	}
}

void NNVAEModelComponent::UpdateOptimizer()
{
	if( m_pOptimizer )
	{
		std::vector<torch::optim::OptimizerParamGroup>&	paramGroups = m_pOptimizer->param_groups();
		for(std::vector<torch::optim::OptimizerParamGroup>::iterator iter = paramGroups.begin(); iter != paramGroups.end(); iter++)
		{
			iter->options().set_lr(m_learningRate);
		}
	}
	else
	{
		torch::optim::AdamWOptions	adamwOptions(m_learningRate);
		adamwOptions.betas() = {0.5, 0.9};	//	these were the betas used the SD repository, but I think that was Adam instead of AdamW and I have a different loss function atm
		m_pOptimizer = new torch::optim::AdamW((*m_pNNVAE)->parameters(true), adamwOptions);
	}
}

void NNVAEModelComponent::SetLearningRate(f32 learningRate)
{
	m_learningRate = learningRate;
}

void NNVAEModelComponent::SetDevice(torch::Device device)
{
	m_deviceToUse = device;

	(*m_pNNVAE)->to(m_deviceToUse);
}

const std::vector<f32>* NNVAEModelComponent::GetLossHistory()
{
	return &m_trainingLossHistory;
}

void NNVAEModelComponent::ResetLossHistory()
{
	m_trainingLossHistory.clear();
	m_validationLossHistory.clear();
}

bool NNVAEModelComponent::RequestTrainingEnd()
{
	return m_requestTrainingEnd;
}

bool NNVAEModelComponent::IsTraining()
{
	return (*m_pNNVAE)->is_training();
}

void NNVAEModelComponent::GetBatch(int batchSize, eBatchType batchType, std::map<std::string, torch::Tensor>* tagToBatchTensorMap)
{
	std::vector<AnimationIntermediateSeq>*	pBatchAnims = (batchType == kBatchType_Validation) ? (&g_animSeqDataSet->m_validationAnims) : (&g_animSeqDataSet->m_trainingAnims);

	std::vector<torch::Tensor>	tensorLists[kNumBatchComponents];
	for(int batchItemIdx = 0; batchItemIdx < batchSize; ++batchItemIdx)
	{
		int							randomAnimIdx = rand() % pBatchAnims->size();
		AnimationIntermediateSeq*	sourceAnimInter = &(*pBatchAnims)[randomAnimIdx];

		//
		{			
			torch::Tensor	wholeAnimImageLocal = sourceAnimInter->animImageC3DR6.GetLocalKeyframeImageForWholeAnim();
			tensorLists[kAnimImage].push_back(wholeAnimImageLocal);

			//torch::Tensor	wholeAnimImageLocal = sourceAnimInter->animImageEuler.GetLocalKeyframeImageForWholeAnim();
			//tensorLists[kAnimImage].push_back(wholeAnimImageLocal);
		}
	}

	for(int batchComponentIdx = 0; batchComponentIdx < kNumBatchComponents; ++batchComponentIdx)
	{
		(*tagToBatchTensorMap)[kTensorLabels[batchComponentIdx]] = torch::stack(tensorLists[batchComponentIdx]);
	}
}