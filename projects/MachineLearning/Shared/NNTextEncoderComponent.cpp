#include "NNTextEncoderComponent.h"

#include "MathNamespace.h"
#include "NNTextEncoder.h"

#include "AnimSeqDataSet.h"
#include "NNTokenizer.h"
#include "NNScheduling.h"

namespace
{
	enum
	{
		kTextEncoder_SampleContextText,
		kTextEncoder_SampleTargetText,

		kNumBatchComponents
	};

	const char*	kTensorLabels[] = 
	{
		"TextEncoder_SampleContextText",
		"TextEncoder_SampleTargetText"
	};

	enum
	{
		kPrintTrainingRate = 10
	};

	const bool	kAutoSaveOnBestValidation = true;
	const int	kValidationAutoSaveIterationThreshold = 5000;
}

NNTextEncoderComponent::NNTextEncoderComponent():
	m_pNNTextEncoder(nullptr),
	m_pOptimizer(nullptr),
	m_deviceToUse(c10::DeviceType::CPU),
	m_learningRate(5.0e-6f),
	m_iterationCount(0),
	m_pScheduler(nullptr)
{
	if( g_animSeqDataSet )
	{
		g_animSeqDataSet->CreateTextEncoderTrainingData();
		m_epochGen.Init();
	}

	m_pNNTextEncoder = new NNTextEncoder();

	int64_t	parameterTensorCount;
	int64_t	parameterElementCount;
	GetParameterCount(*(*m_pNNTextEncoder).get(), &parameterTensorCount, &parameterElementCount);
	DebugPrintf("NNTextEncoderComponent parameter tensor count = %lld, element count = %lld\n", parameterTensorCount, parameterElementCount);

	(*m_pNNTextEncoder)->apply(CommonInitModule);
}

NNTextEncoderComponent::~NNTextEncoderComponent()
{
	delete m_pScheduler;
	delete m_pOptimizer;
	delete m_pNNTextEncoder;
}

void NNTextEncoderComponent::CalcLoss(torch::Tensor contextText, torch::Tensor targetText, torch::Tensor& lossOut)
{
	torch::Tensor		encodedText = (*m_pNNTextEncoder)->forward(contextText);
	torch::Tensor		logits = (*m_pNNTextEncoder)->LanguageModelingForward(encodedText);

	//	cross_entropy apparently expects "channels" to be the second dimension, so we need to change how we view the data to calc loss
	//	logits			= B,T,C
	//	targets			= B,T
	//	logitsConcat	= B*T,C
	//	targetsConcat	= B*T
	torch::Tensor		logitsConcat = logits.view({logits.size(0) * logits.size(1), logits.size(2)});
		
	torch::Tensor		targetsConcat = targetText.view({targetText.size(0) * targetText.size(1)});

	lossOut = torch::nn::functional::cross_entropy(logitsConcat, targetsConcat);
}

/*
	128 max tokens:
	trained at lr = 7.8e-06
	saved at:

	Train NNTextEncoderComponent 34490 : current loss = 0.0105052069, recent loss avg = 0.00776280602, overall avg loss = 0.526478708

	AnimGenSeqProcessingThread::ValidationIteration : current epoch = 0, 28.297048 minutes, 34498 training iterations
	VALIDATION NNTextEncoderComponent : current loss = 0.0788391605, recent loss avg = 0.0854664966, overall average loss = 0.970534682

	Train NNTextEncoderComponent 34500 : current loss = 0.00942652207, recent loss avg = 0.00776814763, overall avg loss = 0.526328444
*/
/*
	256 max tokens:
	trained at lr = 5.0e-06, reduced to 2.0e-06 after some time
	saved at:

	Train NNTextEncoderComponent 84490 : current loss = 0.000743843033, recent loss avg = 0.000886945985, overall avg loss = 0.456884235

	NNProcessingThreadBase::ValidationIteration : current epoch = 0, 121.559585 minutes, 84496 training iterations
	VALIDATION NNTextEncoderComponent : current loss = 0.140456691, recent loss avg = 0.125633746, overall average loss = 1.08019638

	Train NNTextEncoderComponent 84500 : current loss = 0.000934318756, recent loss avg = 0.000886202557, overall avg loss = 0.456830174
*/
void NNTextEncoderComponent::TrainingIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap, bool shouldPrint)
{
	torch::Tensor	loss;
	torch::Tensor	contextText = tagToBatchTensorMap[kTensorLabels[kTextEncoder_SampleContextText]];
	torch::Tensor	targetText = tagToBatchTensorMap[kTensorLabels[kTextEncoder_SampleTargetText]];

	CalcLoss(contextText, targetText, loss);

	//	gradient descent
	bool			set_to_none = true;
	m_pOptimizer->zero_grad(set_to_none);
	loss.backward();
	double	gradNorm = torch::nn::utils::clip_grad_norm_((*m_pNNTextEncoder)->parameters(), 20.0);	//	just so that I can print it atm
	m_pOptimizer->step();

	f32				currentLoss = loss.item().toFloat();
	m_trainingLossHistory.push_back(currentLoss);

	m_iterationCount++;
	UpdateOptimizer();

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

		DebugPrintf("Train NNTextEncoderComponent %d : current loss = %.9g, recent loss avg = %.9g, epoch = %d, gradNorm = %.9g\n", m_iterationCount, currentLoss, recentLossAvg, currentEpoch, gradNorm);
		DebugPrintf("\n");
	}
}

void NNTextEncoderComponent::ValidationIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap)
{
	torch::NoGradGuard	no_grad;

	torch::Tensor	loss;
	torch::Tensor	contextText = tagToBatchTensorMap[kTensorLabels[kTextEncoder_SampleContextText]];
	torch::Tensor	targetText = tagToBatchTensorMap[kTensorLabels[kTextEncoder_SampleTargetText]];

	CalcLoss(contextText, targetText, loss);

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
				(*m_pNNTextEncoder)->train();	//	Save sets the module to eval mode (does it need to?  I should test), so switch back

				DebugPrintf("New best validation saved.\n\n");
			}
		}

		float	lrMult = m_pScheduler->Schedule(m_iterationCount);

		DebugPrintf("VALIDATION NNTextEncoderComponent : current loss = %.9g, recent loss avg = %.9g\n", currentLoss, recentLossAvg);
		DebugPrintf("\tbase learning rate = %.9g, effective learning rate = %.9g, best validation avg seen = %.9g\n", m_learningRate, m_learningRate * lrMult, bestValidationAvg);
		DebugPrintf("\n");
	}
}

void NNTextEncoderComponent::Save()
{
	SaveModule(m_pNNTextEncoder->get(), "NNTextEncoderComponent.pt");
}

void NNTextEncoderComponent::Load()
{
	LoadModule(m_pNNTextEncoder->get(), "NNTextEncoderComponent.pt");
}

void NNTextEncoderComponent::TrainMode()
{
	(*m_pNNTextEncoder)->train();
}

void NNTextEncoderComponent::EvalMode()
{
	(*m_pNNTextEncoder)->eval();
}

void NNTextEncoderComponent::UpdateOptimizer()
{
	if( m_pOptimizer )
	{
		float	lrMult = m_pScheduler->Schedule(m_iterationCount);

		std::vector<torch::optim::OptimizerParamGroup>&	paramGroups = m_pOptimizer->param_groups();
		for(std::vector<torch::optim::OptimizerParamGroup>::iterator iter = paramGroups.begin(); iter != paramGroups.end(); iter++)
		{
			iter->options().set_lr(m_learningRate * lrMult);
		}
	}
	else
	{
		torch::optim::AdamWOptions	adamwOptions(m_learningRate);
		adamwOptions.weight_decay() = 0.1;
		adamwOptions.betas({0.9, 0.95});
		adamwOptions.eps() = 1e-8;
		m_pOptimizer = new torch::optim::AdamW((*m_pNNTextEncoder)->parameters(true), adamwOptions);

		std::vector<int64_t>	warmUpSteps = {1000, 100};
		std::vector<float>		LRMultMin = {0.1f, 0.1f};
		std::vector<float>		LRMultMax = {1.0f, 0.1f};
		std::vector<float>		LRMultWarmUpStart = {1.0e-6f, 0.1f};
		std::vector<int64_t>	cycleLength = {100000, 10000000000};

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

void NNTextEncoderComponent::SetLearningRate(f32 learningRate)
{
	m_learningRate = learningRate;
}

void NNTextEncoderComponent::SetDevice(torch::Device device)
{
	m_deviceToUse = device;

	(*m_pNNTextEncoder)->to(m_deviceToUse);
}

const std::vector<f32>* NNTextEncoderComponent::GetLossHistory()
{
	return &m_trainingLossHistory;
}

void NNTextEncoderComponent::ResetLossHistory()
{
	m_trainingLossHistory.clear();
	m_validationLossHistory.clear();
}

bool NNTextEncoderComponent::IsTraining()
{
	return (*m_pNNTextEncoder)->is_training();
}

void NNTextEncoderComponent::GetBatch(int batchSize, eBatchType batchType, std::map<std::string, torch::Tensor>* tagToBatchTensorMap)
{
	torch::Tensor	dataTensor = (batchType == kBatchType_Validation) ? (g_animSeqDataSet->m_validationText) : (g_animSeqDataSet->m_trainingText);

	Assert(g_NNTokenizer != nullptr);

	int				startToken = g_NNTokenizer->GetStartToken();
	int				endToken = g_NNTokenizer->GetEndToken();

	torch::Tensor	startTokenTensor = torch::tensor({startToken}).to(torch::kInt64);
	torch::Tensor	endTokenTensor = torch::tensor({endToken}).to(torch::kInt64);

	std::vector<torch::Tensor>	tensorLists[kNumBatchComponents];
	for(int batchItemIdx = 0; batchItemIdx < batchSize; ++batchItemIdx)
	{
		const int		kSliceSize = NNTextEncoderImpl::kMaxTokens - 2;
		int				textSamplePosition = 0;
		if( batchType == kBatchType_Validation )
		{
			int				randMin = 0;
			int				randMax = (int)(dataTensor.size(0) - kSliceSize - 1);
			textSamplePosition = rand() % randMax;
		}
		else
		{
			NNTextEncoderEpoch::BatchItemData	batchItemData = m_epochGen.PopNextBatchItem();
			textSamplePosition = batchItemData.m_dataOffset;
		}

		//	we assume that the dataTensor is already tokenized (should be done in dataSet creation)
		torch::Tensor	contextText = dataTensor.slice(0, textSamplePosition, textSamplePosition + kSliceSize);
		torch::Tensor	targetText = dataTensor.slice(0, textSamplePosition + 1, textSamplePosition + kSliceSize + 1);

		contextText = torch::cat({startTokenTensor, contextText, endTokenTensor}, 0);
		targetText = torch::cat({startTokenTensor, targetText, endTokenTensor}, 0);

		tensorLists[kTextEncoder_SampleContextText].push_back(contextText);
		tensorLists[kTextEncoder_SampleTargetText].push_back(targetText);
	}

	for(int batchComponentIdx = 0; batchComponentIdx < kNumBatchComponents; ++batchComponentIdx)
	{
		(*tagToBatchTensorMap)[kTensorLabels[batchComponentIdx]] = torch::stack(tensorLists[batchComponentIdx]);
	}
}