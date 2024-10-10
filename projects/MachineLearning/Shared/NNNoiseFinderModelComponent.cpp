#include "NNNoiseFinderModelComponent.h"

#include "MathNamespace.h"
#include "AnimSeqDataSet.h"

#include "NNVAE.h"
#include "NNDDPMSampler.h"

#include "NNNoiseFinder.h"

#include "NNTextEncoder.h"
#include "NNTokenizer.h"

#include "NNCLIP.h"

#include "AnimDataAug.h"

namespace
{
	enum
	{
		kNoiseFinder_AnimTextDesc,
		kNoiseFinder_AnimImage,
		kNoiseFinder_TimestepIndex,

		kNumBatchComponents
	};

	const char*	kTensorLabels[] = 
	{
		"NoiseFinder_AnimTextDesc",
		"NoiseFinder_AnimImage",
		"NoiseFinder_TimestepIndex"
	};

	//	SD did this, but why?
	//		I suspect the reason has something to do with how the latent aligns with noise, and this creates a more advantageous mix?
	//		reading up, it sounds like it is related to normalizing the latents produced by the encoder so that they have unit variance
	//			thinking about it, this makes sense.  if you are sampling from noise, you want the expected latent values to be obtainable from that normal distribution of noise.
	//const f32	kScaleFactor = 0.18215f;	//	from SD
	const f32	kScaleFactor = 0.365f;		//	from the calc below; interesting that it's about double SD.  make sure to re-evaluate when and if the encoder changes.  might be better if it were slightly smaller?
	const f32	kInvScaleFactor = 1.0f / kScaleFactor;

	enum
	{
		kPrintTrainingRate = 10,
		kSubBatchSize = 2,						//	make sure that this is a power of 2
		kGradClipWarmupSteps = 1000
	};

	const bool	kStopTrainingOnEpoch = false;
	const bool	kAutoSaveOnBestValidation = true;
	const int	kValidationAutoSaveIterationThreshold = 5000;

	const float	kLROffset = 1.0f;
	const int	kBatchIterationOffset = 0;
}

NNNoiseFinderModelComponent::NNNoiseFinderModelComponent():
	m_pNNNoiseFinder(nullptr),
	m_pOptimizer(nullptr),
	m_deviceToUse(c10::DeviceType::CPU),
	//m_learningRate(1.0e-6f),	//	SGD TSE
	//m_learningRate(0.01f),	//	SGD MSE
	//m_learningRate(1.0e-4f),	//	SD AdamW MSE
	//m_learningRate(1.0e-5f),	//	attempt to fix SD method; letting AdamW have a lr much bigger than this causes loss to get stuck around 1
	m_learningRate(1.0e-5f),	//	temp
	m_requestTrainingEnd(false),
	m_iterationCount(0),
	m_pScheduler(nullptr),
	m_currentSubBatch(0),
	m_accumSubBatchLoss(0.0f),
	m_currentBatchSize(kSubBatchSize * 16),
	m_currentSubBatchesPerBatch(m_currentBatchSize / kSubBatchSize),
	m_validationBatchCount(0)
{
	m_pNNNoiseFinder = new NNNoiseFinder();

	int64_t	parameterTensorCount;
	int64_t	parameterElementCount;
	GetParameterCount(*(*m_pNNNoiseFinder).get(), &parameterTensorCount, &parameterElementCount);
	DebugPrintf("NNNoiseFinder parameter tensor count = %lld, element count = %lld\n", parameterTensorCount, parameterElementCount);

	//	relying on base initialization because of some zero_module Conv2 layers in the model
	//(*m_pNNNoiseFinder)->apply(CommonInitModule);

	m_learningRate *= kLROffset;
}

NNNoiseFinderModelComponent::~NNNoiseFinderModelComponent()
{
	delete m_pScheduler;
	delete m_pOptimizer;
	delete m_pNNNoiseFinder;
}

//	the paper says:
//		add noise to source latent at some time
//		predict noise of that result at that time using neural network
//		mse_loss on original vs predicted noise
void NNNoiseFinderModelComponent::TrainingIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap, bool shouldPrint)
{
	//Assert(g_NNTextEncoder != nullptr);
	Assert(g_NNCLIP != nullptr);
	Assert(g_NNVAE != nullptr);
	Assert(g_NNDDPMSampler != nullptr);

	torch::Tensor		animTextDesc = tagToBatchTensorMap[kTensorLabels[kNoiseFinder_AnimTextDesc]];
	torch::Tensor		inputAnimData = tagToBatchTensorMap[kTensorLabels[kNoiseFinder_AnimImage]];
	torch::Tensor		timestepData = tagToBatchTensorMap[kTensorLabels[kNoiseFinder_TimestepIndex]].squeeze(-1);

	torch::Tensor		loss;
	torch::Tensor		encodedText;
	torch::Tensor		logVar;
	torch::Tensor		mean;
	torch::Tensor		latentAnimData;
	std::string			lossLog;

	{
		torch::NoGradGuard	no_grad;
		
		//encodedText = g_NNTextEncoder->forward(animTextDesc);
		encodedText = g_NNCLIP->forward(animTextDesc);

		latentAnimData = g_NNVAE->EncoderForward(inputAnimData, &logVar, &mean);

		//	try to estimate a scale factor I guess?
		/*static torch::Tensor	allLatentsTest;

		if( allLatentsTest.defined() )
		{
			allLatentsTest = torch::cat({allLatentsTest, latentAnimData}, 0);
		}
		else
		{
			allLatentsTest = latentAnimData;
		}

		if( shouldPrint )
		{
			torch::Tensor	latentStdDev = allLatentsTest.std();
			DebugPrintf("scale estimate = %.9g\n", 1.0f / latentStdDev.item().toFloat());
		}*/

		latentAnimData = latentAnimData * kScaleFactor;
	}

	//	create the base noise
	//	this is epsilon in equation 14
	torch::Tensor		baseNoise = torch::randn(latentAnimData.sizes(), latentAnimData.dtype()).to(latentAnimData.device());

	//	add noise to the base latent desc at this time.  I think this is supposed to produce the latent desc after adding the given noise over that much time?
	//	this is the first input to epsilon_theta in equation 14
	torch::Tensor		noisyLatentDesc = g_NNDDPMSampler->QSample(latentAnimData, baseNoise, timestepData);

	//at::autocast::set_enabled(true);
	//	use the network to predict the noise at this time
	//	this is the epsilon_theta term in equation 14 in the paper
	torch::Tensor		predictedNoise = (*m_pNNNoiseFinder)->forward(noisyLatentDesc, encodedText, timestepData);

	//	calc loss
	(*m_pNNNoiseFinder)->CalcLoss(predictedNoise, baseNoise, timestepData, &loss, &lossLog);
	//at::autocast::set_enabled(false);

	//	gradient accumulation
	if( m_currentSubBatch == 0 )
	{
		bool				set_to_none = true;
		m_pOptimizer->zero_grad(set_to_none);

		m_accumSubBatchLoss = 0.0f;
	}

	loss = loss * (1.0f / (float)m_currentSubBatchesPerBatch);	//	scale the loss down so that we average sub-batch gradients correctly
	loss.backward();
	m_currentSubBatch++;

	m_accumSubBatchLoss += loss.item().toFloat();

	if( m_currentSubBatch >= m_currentSubBatchesPerBatch )
	{
		m_currentSubBatch = 0;
		//m_currentSubBatchesPerBatch = 3;
		//m_currentSubBatchesPerBatch = (m_iterationCount+kBatchIterationOffset)*7/12500 + 3;		//	scale up the batch size as we go along
		//m_currentBatchSize = m_currentSubBatchesPerBatch * kSubBatchSize;

		double	gradNorm = 1.0;
		//if( m_iterationCount > kGradClipWarmupSteps )
		//{
		//	//	do I actually want to do this?  it slows down training substantially if we do it too early; it is possible that that is a good thing though.
		//	//		is the point of the warmup on the learning rate scheduler a way to try to keep this low in the early steps?
		//	//			so a large gradNorm early indicates needing more warmup steps?
		//	//			or a need to lower the learning rate?
		//	//			or a larger batch size?
			//gradNorm = torch::nn::utils::clip_grad_norm_((*m_pNNNoiseFinder)->parameters(), 1.0);	//	clip gradients to max norm 1
		//}
		//else
		//{
			gradNorm = torch::nn::utils::clip_grad_norm_((*m_pNNNoiseFinder)->parameters(), 20.0);	//	this is really just here so that I can log the norm
			//gradNorm = torch::nn::utils::clip_grad_norm_((*m_pNNNoiseFinder)->parameters(), 2.0);
		//}

		//
		m_pOptimizer->step();

		m_trainingLossHistory.push_back(m_accumSubBatchLoss);

		m_iterationCount++;
		UpdateOptimizer();

		int	currentEpoch = m_epochGen.GetCreationCycles();

		if( shouldPrint && 
			(m_iterationCount % kPrintTrainingRate == 0) )
		{
			f32	recentLossAvg = 0;
			int	avgThreshold = 100;
			int	itemCount = std::min(avgThreshold, (int)m_trainingLossHistory.size());
			int	startLoop = (int)m_trainingLossHistory.size()-1;
			int	endLoop = startLoop - itemCount;
			for(int i = startLoop; i > endLoop; --i)
			{
				recentLossAvg += m_trainingLossHistory[i];
			}
			recentLossAvg /= itemCount;

			/*f32	recentVariance = 0;
			for(int i = startLoop; i > endLoop; --i)
			{
				f32		thisSE = (m_trainingLossHistory[i] - recentLossAvg);
				thisSE = thisSE * thisSE;
				recentVariance += thisSE;
			}
			recentVariance /= itemCount;

			f32	recentStdDev = sqrt(recentVariance);*/

			DebugPrintf("Train NNNoiseFinderModelComponent %d : current loss = %.9g, recent loss avg = %.9g, epoch = %d, gradNorm = %.9g, batch size = %d\n", m_iterationCount, m_accumSubBatchLoss, recentLossAvg, currentEpoch, gradNorm, m_currentBatchSize);
			//DebugPrintf("\t%s\n", lossLog.c_str());	//	this would need to be updated to work with sub-batches (maybe list the loss for each sub-batch?)
			DebugPrintf("\n");
		}

		if( kStopTrainingOnEpoch &&
			currentEpoch > 1 )
		{
			DebugPrintf("Epoch finished.  Stopping training.\n\n");
			m_requestTrainingEnd = true;
		}
	}
}

void NNNoiseFinderModelComponent::ValidationIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap)
{
	torch::NoGradGuard	no_grad;

	torch::Tensor		animTextDesc = tagToBatchTensorMap[kTensorLabels[kNoiseFinder_AnimTextDesc]];
	torch::Tensor		inputAnimData = tagToBatchTensorMap[kTensorLabels[kNoiseFinder_AnimImage]];
	torch::Tensor		timestepData = tagToBatchTensorMap[kTensorLabels[kNoiseFinder_TimestepIndex]].squeeze(-1);

	torch::Tensor		loss;

	torch::Tensor		encodedText;

	torch::Tensor		logVar;
	torch::Tensor		mean;
	torch::Tensor		latentAnimData;
	std::string			lossLog;

	{
		//encodedText = g_NNTextEncoder->forward(animTextDesc);
		encodedText = g_NNCLIP->forward(animTextDesc);

		latentAnimData = g_NNVAE->EncoderForward(inputAnimData, &logVar, &mean);
		latentAnimData = latentAnimData * kScaleFactor;
	}

	torch::Tensor		baseNoise = torch::randn(latentAnimData.sizes(), latentAnimData.dtype()).to(latentAnimData.device());

	torch::Tensor		noisyLatentDesc = g_NNDDPMSampler->QSample(latentAnimData, baseNoise, timestepData);

	torch::Tensor		predictedNoise = (*m_pNNNoiseFinder)->forward(noisyLatentDesc, encodedText, timestepData);
	(*m_pNNNoiseFinder)->CalcLoss(predictedNoise, baseNoise, timestepData, &loss, &lossLog);

	{
		f32			currentLoss = loss.item().toFloat();
		m_validationLossHistory.push_back(currentLoss);

		f32	recentLossAvg = 0;
		int	avgThreshold = 30;
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

		//
		static f32		bestValidationAvg = 99999999999999999.0f;
		//const float		kLearningRateLowerBound = 1.0e-9f;

		//	method based on how long it has been since the best validation avg was recorded
		const int		kBestValidationAvgThreshold = 20;
		static float	s_iterationsSinceBestValidationLossAvg = 0;
		if( recentLossAvg < bestValidationAvg )
		{
			bestValidationAvg = recentLossAvg;
			s_iterationsSinceBestValidationLossAvg = 0;

			if( kAutoSaveOnBestValidation &&
				m_iterationCount > kValidationAutoSaveIterationThreshold )
			{
				Save();
				(*m_pNNNoiseFinder)->train();	//	Save sets the module to eval mode (does it need to?  I should test), so switch back

				DebugPrintf("New best validation saved.\n\n");
			}
		}
		//else
		//{
		//	s_iterationsSinceBestValidationLossAvg++;
		//	if( s_iterationsSinceBestValidationLossAvg > kBestValidationAvgThreshold )
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

		//		//	reset the counter and hope things get better with the lower learning rate
		//		s_iterationsSinceBestValidationLossAvg = 0;
		//	}
		//}

		//	nan check
		if( currentLoss != currentLoss )
		{
			DebugPrintf("NaN loss detected.  Stopping training.\n\n");
			m_requestTrainingEnd = true;
		}

		float	lrMult = m_pScheduler->Schedule(m_iterationCount);

		//	print log
		DebugPrintf("VALIDATION NNNoiseFinderModelComponent : current loss = %.9g, recent loss avg = %.9g, recent loss stddev = %.9g\n", currentLoss, recentLossAvg, recentStdDev);
		//DebugPrintf("\t%s\n", lossLog.c_str());
		DebugPrintf("\tbase learning rate = %.9g, effective learning rate = %.9g, best validation avg seen = %.9g\n", m_learningRate, m_learningRate * lrMult, bestValidationAvg);
		DebugPrintf("\n");

		//
		if( m_pScheduler->Finished(m_iterationCount) )
		{
			DebugPrintf("Final scheduler iteration reached.  Stopping training.\n\n");
			m_requestTrainingEnd = true;
		}
	}
}

void NNNoiseFinderModelComponent::Save()
{
	SaveModule(m_pNNNoiseFinder->get(), "NNNoiseFinder.pt");
}

void NNNoiseFinderModelComponent::Load()
{
	LoadModule(m_pNNNoiseFinder->get(), "NNNoiseFinder.pt");
}

void NNNoiseFinderModelComponent::TrainMode()
{
	m_requestTrainingEnd = false;
	(*m_pNNNoiseFinder)->train();

	/*std::vector<torch::Tensor>	modelParams = (*m_pNNNoiseFinder)->parameters(true);
	for(std::vector<torch::Tensor>::iterator iter = modelParams.begin(); iter != modelParams.end(); iter++)
	{
		iter->set_requires_grad(true);
	}*/
}

void NNNoiseFinderModelComponent::EvalMode()
{
	(*m_pNNNoiseFinder)->eval();

	/*std::vector<torch::Tensor>	modelParams = (*m_pNNNoiseFinder)->parameters(true);
	for(std::vector<torch::Tensor>::iterator iter = modelParams.begin(); iter != modelParams.end(); iter++)
	{
		iter->set_requires_grad(false);
	}*/
}

void NNNoiseFinderModelComponent::UpdateOptimizer()
{
	if( m_pOptimizer )
	{
		float	lrMult = m_pScheduler->Schedule(m_iterationCount);	//	is this supposed to be something bigger than iterations?

		std::vector<torch::optim::OptimizerParamGroup>&	paramGroups = m_pOptimizer->param_groups();
		for(std::vector<torch::optim::OptimizerParamGroup>::iterator iter = paramGroups.begin(); iter != paramGroups.end(); iter++)
		{
			iter->options().set_lr(m_learningRate * lrMult);
		}

		/*std::vector<torch::optim::OptimizerParamGroup>&	paramGroups = m_pOptimizer->param_groups();
		for(std::vector<torch::optim::OptimizerParamGroup>::iterator iter = paramGroups.begin(); iter != paramGroups.end(); iter++)
		{
			iter->options().set_lr(m_learningRate);
		}*/
	}
	else
	{
		//	AdamW seems to fail with learning rates much above 1.0e-5f
		//		also seems to take about 4 times as long to iterate as SGD and 1.5 times the VRAM
		torch::optim::AdamWOptions	adamwOptions(m_learningRate);
		//adamwOptions.weight_decay() = 0.1;
		//adamwOptions.betas({0.9, 0.95});
		//adamwOptions.eps() = 1e-8;
		m_pOptimizer = new torch::optim::AdamW((*m_pNNNoiseFinder)->parameters(true), adamwOptions);

		//	a cursory look at Adam wasn't better
		//torch::optim::AdamOptions	adamOptions(m_learningRate);
		//m_pOptimizer = new torch::optim::Adam((*m_pNNNoiseFinder)->parameters(true), adamOptions);

		//	but SGD seems to work more generally
		//m_pOptimizer = new torch::optim::SGD((*m_pNNNoiseFinder)->parameters(true), m_learningRate);

		//	SGD with stuff
		//torch::optim::SGDOptions	sgdOptions(m_learningRate);
		//sgdOptions.momentum() = 0.9;
		//sgdOptions.weight_decay() = 1e-2;
		//m_pOptimizer = new torch::optim::SGD((*m_pNNNoiseFinder)->parameters(true), sgdOptions);

		//	scheduler

		//
		std::vector<int64_t>	warmUpSteps = {10000};
		std::vector<float>		LRMultMin = {1.0f};
		std::vector<float>		LRMultMax = {1.0f};
		std::vector<float>		LRMultWarmUpStart = {1.0e-6f};
		std::vector<int64_t>	cycleLength = {10000000000};

		//
		/*std::vector<int64_t>	warmUpSteps = {1000, 100};
		std::vector<float>		LRMultMin = {0.1f, 0.1f};
		std::vector<float>		LRMultMax = {1.0f, 0.1f};
		std::vector<float>		LRMultWarmUpStart = {1.0e-6f, 0.1f};
		std::vector<int64_t>	cycleLength = {10000, 10000000000};*/

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

void NNNoiseFinderModelComponent::SetLearningRate(f32 learningRate)
{
	m_learningRate = learningRate;
}

void NNNoiseFinderModelComponent::SetDevice(torch::Device device)
{
	m_deviceToUse = device;

	(*m_pNNNoiseFinder)->to(m_deviceToUse);
}

const std::vector<f32>* NNNoiseFinderModelComponent::GetLossHistory()
{
	return &m_trainingLossHistory;
}

void NNNoiseFinderModelComponent::ResetLossHistory()
{
	m_trainingLossHistory.clear();
	m_validationLossHistory.clear();
}

bool NNNoiseFinderModelComponent::RequestTrainingEnd()
{
	return m_requestTrainingEnd;
}

bool NNNoiseFinderModelComponent::IsTraining()
{
	return (*m_pNNNoiseFinder)->is_training();
}

int NNNoiseFinderModelComponent::SubBatchSize()
{
	return kSubBatchSize;
}

void NNNoiseFinderModelComponent::GetBatch(int batchSize, eBatchType batchType, std::map<std::string, torch::Tensor>* tagToBatchTensorMap)
{
	Assert(batchSize > 0);

	std::vector<AnimationIntermediateSeq>*	pBatchAnims = (batchType == kBatchType_Validation) ? (&g_animSeqDataSet->m_validationAnims) : (&g_animSeqDataSet->m_trainingAnims);

	Assert(g_NNTokenizer != nullptr);

	std::vector<torch::Tensor>	tensorLists[kNumBatchComponents];
	for(int batchItemIdx = 0; batchItemIdx < batchSize; ++batchItemIdx)
	{
		int							targetAnimIdx = 0;
		int							targetTimestep = 0;
		bool						trainCFG = true;

		if( batchType == kBatchType_Validation )
		{
			//	making the validation loss consistent is nice as long as we're autosaving based on the metric
			//	would be really nice to increase the validation batch size to resolve a lot of this, but not really viable without some special scheme

			//	doing things this way is probably more consistent, but it makes the loss numbers much different from training, so we can't really compare them
			//targetAnimIdx = batchItemIdx % pBatchAnims->size();	//	use the same animations every time; kind of a waste to only use 2 animations for validation?  will find good save points for those though.
			//targetAnimIdx = rand() % pBatchAnims->size();			//	random animations are way less consistent but use them all I guess
			int		targetAnimOffset = m_validationBatchCount * batchSize;
			targetAnimIdx = targetAnimOffset % pBatchAnims->size();	//	maybe this will be kind of consistent and use all of the animations?  it will take a while to cycle through them all though so might miss a good save point.
			targetTimestep = batchItemIdx;	//	empirically, it seems like the lowest timesteps have the hardest time training, so let's just grab the bottom {batch} timesteps

			//	this would probably give numbers closer to what we get in training, but be less consistent (if batch size is high enough it may be consistent enough anyway)
			//		as long as we're doing validation batch == 2 (subbatch size) that's prob not good enough
			//targetAnimIdx = rand() % pBatchAnims->size();
			//targetTimestep = rand() % g_NNDDPMSampler2->GetTrainingStepCount();

			//	always use the conditioned prompt in validation for consistency?
			//		could also have some scheme where we turn it on and off based on batch item idx
			trainCFG = false;
		}
		else
		{
			NNNoiseFinderEpoch::BatchItemData	batchItemData = m_epochGen.PopNextBatchItem();

			//targetAnimIdx = 0;	//	test with only one animation to quickly see if the network can actually represent it
			targetAnimIdx = batchItemData.m_animIdx;
			targetTimestep = batchItemData.m_timestepIdx;
		}

		AnimationIntermediateSeq*	pSourceAnimInter = &(*pBatchAnims)[targetAnimIdx];

		//	description string
		{
			std::string		animTextDescString = pSourceAnimInter->descString;

			if( trainCFG )
			{
				//	train the "unconditioned" part of the model by randomly excluding the description text
				const float	kUncondChance = 0.1f;
				float		uncondRoll = Math::FRand();
				if( uncondRoll < kUncondChance )
				{
					animTextDescString = "";
				}
			}

			//	tokenize the prompt and make it a tensor
			torch::Tensor	animTextDescTokenized = g_NNTokenizer->EncodePaddedTensor(animTextDescString, NNCLIPImpl::kMaxTokens, true, true);
			tensorLists[kNoiseFinder_AnimTextDesc].push_back(animTextDescTokenized);
		}

		//	timestep
		{
			//	pick a time index to add noise at
			//	this is t in equation 14
			torch::Tensor	timestep = torch::tensor({targetTimestep}, torch::kInt64);
			tensorLists[kNoiseFinder_TimestepIndex].push_back(timestep);
		}

		//	animation data
		{			
			torch::Tensor	wholeAnimImageLocal = pSourceAnimInter->animImageC3DR6.GetLocalKeyframeImageForWholeAnim();
			tensorLists[kNoiseFinder_AnimImage].push_back(wholeAnimImageLocal);

			//torch::Tensor	wholeAnimImageWorld = pSourceAnimInter->animImageC3DR6.GetWorldKeyframeImageForWholeAnim();
			//tensorLists[kNoiseFinder_AnimImage].push_back(wholeAnimImageWorld);
		}
	}

	if( batchType == kBatchType_Validation )
	{
		m_validationBatchCount++;
	}

	for(int batchComponentIdx = 0; batchComponentIdx < kNumBatchComponents; ++batchComponentIdx)
	{
		(*tagToBatchTensorMap)[kTensorLabels[batchComponentIdx]] = torch::stack(tensorLists[batchComponentIdx]);
	}
}

//	the paper says
//		go backward in timesteps vs training (from t to 0, with 0 being fully de-noised)
//		for each timestep :
//			predict noise in current latent using neural network
//			remove predicted noise from current latent
torch::Tensor NNNoiseFinderModelComponent::Sample(torch::Tensor input, torch::Tensor encodedPrompt, GenProgressFunc progressFunc)
{
	int					batchSize = (int)input.size(0);

	Assert(encodedPrompt.size(0) == batchSize);

	torch::Tensor		outputLatentDesc = input;

	int	numTimeSteps = g_NNDDPMSampler->GetInferenceTimestepCount();
	for(int i = 0; i < numTimeSteps; ++i)
	{
		torch::Tensor	timeStep = g_NNDDPMSampler->GetInferenceTimestepsElement(i);
		timeStep = timeStep.to(m_deviceToUse);

		torch::Tensor	timeInput = timeStep.expand({batchSize});
		
		torch::Tensor	modelInput = outputLatentDesc;

		//	modelOutput is the predicted noise of the modelInput at this time
		torch::Tensor	predictedNoise = (*m_pNNNoiseFinder)->forward(modelInput, encodedPrompt, timeInput);

		//	this produces the next de-noised sample in the series moving back through time (X(t-1) calculated from X(t) per the paper)
		outputLatentDesc = g_NNDDPMSampler->PSample(modelInput, timeStep, predictedNoise);

		if( progressFunc )
		{
			progressFunc();
		}
	}

	//	this should be the fully de-noised sample at t=0
	return outputLatentDesc;
}

torch::Tensor NNNoiseFinderModelComponent::SampleFromLatent(torch::Tensor inputLatentAnimDesc, torch::Tensor encodedPrompt, GenProgressFunc progressFunc)
{
	torch::Tensor		outputLatentDesc = inputLatentAnimDesc * kScaleFactor;

	outputLatentDesc = Sample(outputLatentDesc, encodedPrompt, progressFunc);

	return outputLatentDesc * kInvScaleFactor;
}

torch::Tensor NNNoiseFinderModelComponent::SampleFromNoise(torch::Tensor inputNoise, torch::Tensor encodedPrompt, GenProgressFunc progressFunc)
{
	torch::Tensor		outputLatentDesc = inputNoise;

	outputLatentDesc = Sample(outputLatentDesc, encodedPrompt, progressFunc);

	return outputLatentDesc * kInvScaleFactor;
}

torch::Tensor NNNoiseFinderModelComponent::SampleWithCFG(torch::Tensor input, torch::Tensor encodedPrompt, torch::Tensor encodedNegativePrompt, double scaleCFG, GenProgressFunc progressFunc)
{
	int					batchSize = (int)input.size(0);

	Assert(encodedPrompt.size(0) == batchSize);
	Assert(encodedNegativePrompt.size(0) == batchSize);

	torch::Tensor		outputLatentDesc = input;

	int	numTimeSteps = g_NNDDPMSampler->GetInferenceTimestepCount();
	for(int i = 0; i < numTimeSteps; ++i)
	{
		torch::Tensor	timeStep = g_NNDDPMSampler->GetInferenceTimestepsElement(i);
		timeStep = timeStep.to(m_deviceToUse);
		
		//	duplicate the inputs along the batch dimension
		torch::Tensor	latentInput = outputLatentDesc;
		torch::Tensor	timeInput = timeStep;

		//	get the noise for both prompts
		torch::Tensor	predictedCondNoise = (*m_pNNNoiseFinder)->forward(latentInput, encodedPrompt, timeInput);
		torch::Tensor	predictedUncondNoise = (*m_pNNNoiseFinder)->forward(latentInput, encodedNegativePrompt, timeInput);

		//	the example I saw was doing the mix this way
		torch::Tensor	predictedNoise = scaleCFG * (predictedCondNoise - predictedUncondNoise) + predictedUncondNoise;

		//	but the paper was doing the mix this way?
		//torch::Tensor	predictedNoise = scaleCFG * (predictedCondNoise - predictedUncondNoise) + predictedCondNoise;

		//	this produces the next de-noised sample in the series moving back through time (X(t-1) calculated from X(t) per the paper)
		outputLatentDesc = g_NNDDPMSampler->PSample(outputLatentDesc, timeStep, predictedNoise);

		if( progressFunc )
		{
			progressFunc();
		}
	}

	//	this should be the fully de-noised sample at t=0
	return outputLatentDesc;
}

torch::Tensor NNNoiseFinderModelComponent::SampleFromNoiseWithCFG(torch::Tensor inputNoise, torch::Tensor encodedPrompt, torch::Tensor encodedNegativePrompt, double scaleCFG, GenProgressFunc progressFunc)
{
	torch::Tensor		outputLatentDesc = inputNoise;

	outputLatentDesc = SampleWithCFG(outputLatentDesc, encodedPrompt, encodedNegativePrompt, scaleCFG, progressFunc);

	return outputLatentDesc * kInvScaleFactor;
}